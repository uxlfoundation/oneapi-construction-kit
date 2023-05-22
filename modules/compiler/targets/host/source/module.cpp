// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <base/module.h>
#include <base/pass_pipelines.h>
#include <clang/AST/ASTContext.h>
#include <clang/Basic/LangStandard.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Serialization/ASTReader.h>
#include <clang/Serialization/ASTRecordReader.h>
#include <compiler/limits.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/cl_builtin_info.h>
#include <compiler/utils/compute_local_memory_usage_pass.h>
#include <compiler/utils/encode_kernel_metadata_pass.h>
#include <compiler/utils/llvm_global_mutex.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/metadata_analysis.h>
#include <compiler/utils/pass_machinery.h>
#include <compiler/utils/reduce_to_function_pass.h>
#include <compiler/utils/simple_callback_pass.h>
#include <compiler/utils/unique_opaque_structs_pass.h>
#include <host/compiler_kernel.h>
#include <host/device.h>
#include <host/host_mux_builtin_info.h>
#include <host/host_pass_machinery.h>
#include <host/module.h>
#include <host/passes.h>
#include <host/target.h>
#include <llvm-c/BitWriter.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/CrashRecoveryContext.h>
#include <llvm/Transforms/IPO/GlobalDCE.h>
#include <multi_llvm/optional_helper.h>
#include <mux/mux.hpp>

#include <cstdint>
#ifndef NDEBUG
#include <llvm/IR/Verifier.h>
#endif
#include <base/base_pass_machinery.h>
#include <cargo/argument_parser.h>
#include <cargo/small_vector.h>
#include <cargo/string_view.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <multi_llvm/multi_llvm.h>

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <unordered_set>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

namespace host {
HostModule::HostModule(compiler::BaseTarget &target,
                       compiler::BaseContext &context, uint32_t &num_errors,
                       std::string &log)
    : BaseModule(target, context, num_errors, log) {}

const HostTarget &HostModule::getHostTarget() const {
  return *static_cast<HostTarget *>(&target);
}

cargo::expected<cargo::dynamic_array<uint8_t>, compiler::Result>
HostModule::hostCompileObject(HostTarget &target,
                              const compiler::Options &build_options,
                              llvm::Module *module) {
  std::unique_ptr<llvm::Module> cloned_module(llvm::CloneModule(*module));

  if (nullptr == cloned_module) {
    return cargo::make_unexpected(compiler::Result::OUT_OF_MEMORY);
  }

  auto pass_mach = createPassMachinery();
  initializePassMachineryForFinalize(*pass_mach);

  llvm::ModulePassManager pm;
  pm.addPass(compiler::utils::TransferKernelMetadataPass());

  pm.addPass(hostGetKernelPasses(build_options, pass_mach->getPB(), snapshots));
  {
    // Using the CrashRecoveryContext and statistics touches LLVM's global
    // state.
    std::lock_guard<std::mutex> globalLock(
        compiler::utils::getLLVMGlobalMutex());

    llvm::CrashRecoveryContext CRC;
    llvm::CrashRecoveryContext::Enable();
    bool crashed =
        !CRC.RunSafely([&] { pm.run(*cloned_module, pass_mach->getMAM()); });
    llvm::CrashRecoveryContext::Disable();
    if (crashed) {
      return cargo::make_unexpected(compiler::Result::FINALIZE_PROGRAM_FAILURE);
    }

    if (llvm::AreStatisticsEnabled()) {
      llvm::PrintStatistics();
    }
  }

  auto binaryOrError =
      emitBinary(cloned_module.get(), target.engine->getTargetMachine());

  if (!binaryOrError.has_value()) {
    return cargo::make_unexpected(binaryOrError.error());
  }
  return std::move(binaryOrError.value());
}

compiler::Result HostModule::createBinary(
    cargo::array_view<std::uint8_t> &buffer) {
  if (!finalized_llvm_module) {
    return compiler::Result::FINALIZE_PROGRAM_FAILURE;
  }

  if (!object_code.empty()) {
    buffer = cargo::array_view<std::uint8_t>(object_code);
    return compiler::Result::SUCCESS;
  }

  auto &host_target = static_cast<HostTarget &>(target);

  std::unique_ptr<llvm::Module> clonedModule;
  {
    std::lock_guard<compiler::Context> guard(context);

    clonedModule = llvm::CloneModule(*finalized_llvm_module.get());

    llvm::SmallVector<char, 1024> object_code_buffer;
    llvm::raw_svector_ostream stream(object_code_buffer);

    auto binaryOrError =
        hostCompileObject(host_target, options, clonedModule.get());
    if (!binaryOrError.has_value()) {
      return binaryOrError.error();
    }

    object_code = std::move(binaryOrError.value());
  }

  buffer = cargo::array_view<std::uint8_t>(object_code);

  return compiler::Result::SUCCESS;
}

llvm::ModulePassManager HostModule::getLateTargetPasses(
    compiler::utils::PassMachinery &) {
  if (options.llvm_stats) {
    llvm::EnableStatistics();
  }
  // We may have a situation where there were already opaque structs in the
  // context associated with HostTarget which have the same name as those
  // in the deserialized module. LLVM tries to resolve this name clash by
  // introducing suffixes to the opaque structs in the deserialized module e.g.
  // __mux_dma_event_t becomes __mux_dma_event_t.0. This is a problem since
  // later passes may rely on the name __mux_dma_event_t to identify the type,
  // so here we remap the structs.
  llvm::ModulePassManager pm;
  pm.addPass(compiler::utils::UniqueOpaqueStructsPass());
  pm.addPass(compiler::utils::SimpleCallbackPass([](llvm::Module &m) {
    // the llvm identifier is no longer needed by our host code
    if (auto md = m.getNamedMetadata("llvm.ident")) {
      md->dropAllReferences();
      md->eraseFromParent();
    }
  }));
#ifdef CA_ENABLE_DEBUG_SUPPORT
  pm.addPass(compiler::utils::SimpleCallbackPass([&](llvm::Module &m) {
    // Custom host options are set as module metadata as a testing aid
    auto &ctx = m.getContext();
    if (options.device_args.data()) {
      if (auto md = m.getOrInsertNamedMetadata("host.build_options")) {
        llvm::MDString *md_string =
            llvm::MDString::get(ctx, options.device_args.data());
        md->addOperand(llvm::MDNode::get(ctx, md_string));
      }
    }
  }));
#endif  // CA_ENABLE_DEBUG_SUPPORT
  return pm;
}

compiler::Kernel *HostModule::createKernel(const std::string &name) {
  std::unique_ptr<llvm::Module> kernel_module;
  handler::GenericMetadata kernel_md(name, name, 0);
  {
    std::lock_guard<compiler::Context> guard(context);
    kernel_module = llvm::CloneModule(*finalized_llvm_module);

    if (!kernel_module || !kernel_module->getFunction(name)) {
      return nullptr;
    }

    llvm::ModulePassManager pm;
    auto pass_mach = createPassMachinery();
    pass_mach->initializeStart();
    pass_mach->initializeFinish();

    // Set up the kernel metadata which informs later passes which kernel we're
    // interested in optimizing.
    compiler::utils::EncodeKernelMetadataPassOptions pass_opts{name};
    pm.addPass(compiler::utils::EncodeKernelMetadataPass(pass_opts));
    pm.addPass(compiler::utils::ReduceToFunctionPass());
    pm.addPass(compiler::utils::ComputeLocalMemoryUsagePass());

    pm.run(*kernel_module, pass_mach->getMAM());
    // Retrieve the estimation of the amount of local memory this kernel uses.
    if (auto *f = kernel_module->getFunction(name)) {
      kernel_md = pass_mach->getFAM()
                      .getResult<compiler::utils::GenericMetadataAnalysis>(*f);
    }
  }
  auto device_info = target.getCompilerInfo()->device_info;
  // These default local sizes are fairly arbitrary, at the moment the key
  // point is that they are greater than 1 to ensure that the vectorizer,
  // barrier code, and local work items scheduling are used.  We work best with
  // powers of two.
  std::array<size_t, 3> local_sizes;
  local_sizes[0] = std::min(64u, device_info->max_work_group_size_x);
  local_sizes[1] = std::min(4u, device_info->max_work_group_size_y);
  local_sizes[2] = std::min(4u, device_info->max_work_group_size_z);

  assert(kernel_md.local_memory_usage <= SIZE_MAX);
  auto kernel = new HostKernel(
      static_cast<HostTarget &>(target), getOptions(), snapshots,
      kernel_module.release(), kernel_md.kernel_name, local_sizes,
      static_cast<size_t>(kernel_md.local_memory_usage));
  return kernel;
}

std::unique_ptr<compiler::utils::PassMachinery>
HostModule::createPassMachinery() {
  auto *TM = static_cast<HostTarget &>(target).engine->getTargetMachine();
  auto Info =
      compiler::initDeviceInfoFromMux(target.getCompilerInfo()->device_info);
  auto Callback = [BI = target.getBuiltins()](const llvm::Module &) {
    return compiler::utils::BuiltinInfo(
        std::make_unique<HostBIMuxInfo>(),
        compiler::utils::createCLBuiltinInfo(BI));
  };
  return std::make_unique<host::HostPassMachinery>(
      target.getLLVMContext(), TM, Info, Callback,
      target.getContext().isLLVMVerifyEachEnabled(),
      target.getContext().getLLVMDebugLoggingLevel(),
      target.getContext().isLLVMTimePassesEnabled());
}

void initializePassMachineryForFinalize(
    compiler::utils::PassMachinery &passMach, const HostTarget &target) {
  auto *TM = target.engine->getTargetMachine();

  passMach.initializeStart();
  if (TM) {
    passMach.getFAM().registerPass(
        [TM] { return llvm::TargetIRAnalysis(TM->getTargetIRAnalysis()); });
  }
  // Ensure that the optimizer doesn't inject calls to library functions that
  // can't be supported on a free-standing device.
  //
  // We cannot use PassManagerBuilder::LibraryInfo here, since the analysis has
  // to be added to the pass manager prior to other passes being added. This is
  // because other passes might require TargetLibraryInfoWrapper, and if they do
  // a TargetLibraryInfoImpl object with default settings will be created prior
  // to adding the pass. Trying to add a TargetLibraryInfoWrapper analysis with
  // disabled functions later will have no affect, due to the analysis already
  // being registered with the pass manager.
  auto Triple = target.engine->getTargetMachine()->getTargetTriple();
  auto LibraryInfo = llvm::TargetLibraryInfoImpl(Triple);
  LibraryInfo.disableAllFunctions();
  passMach.getFAM().registerPass(
      [&LibraryInfo] { return llvm::TargetLibraryAnalysis(LibraryInfo); });
  passMach.initializeFinish();
}

void HostModule::initializePassMachineryForFinalize(
    compiler::utils::PassMachinery &passMach) const {
  return host::initializePassMachineryForFinalize(passMach, getHostTarget());
}

}  // namespace host
