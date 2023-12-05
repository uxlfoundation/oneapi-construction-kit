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
{% if cookiecutter.copyright_name != "" -%}
/// Additional changes Copyright (C) {{cookiecutter.copyright_name}}. All Rights
/// Reserved.
{% endif -%}

#include <base/macros.h>
#include <base/pass_pipelines.h>
#include <compiler/utils/cl_builtin_info.h>
#include <compiler/utils/lld_linker.h>
#include <compiler/utils/llvm_global_mutex.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/CrashRecoveryContext.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Process.h>
#include <llvm/Target/TargetMachine.h>
#include <multi_llvm/multi_llvm.h>
#include <{{cookiecutter.target_name}}/module.h>
#include <{{cookiecutter.target_name}}/target.h>
#include <vecz/pass.h>
#include <{{cookiecutter.target_name}}/{{cookiecutter.target_name}}_pass_machinery.h>

{% if "clmul"  in cookiecutter.feature.split(";") -%}
#include <compiler/utils/mangling.h>
#include <llvm/IR/IntrinsicsRISCV.h>

namespace {{cookiecutter.target_name}} {
  class CLTargetBuiltinInfo : public compiler::utils::CLBuiltinInfo {
   public:
    // Will be filled out by each example
    CLTargetBuiltinInfo(std::unique_ptr<compiler::utils::CLBuiltinLoader> L)
        : compiler::utils::CLBuiltinInfo(std::move(L)) {}
    compiler::utils::Builtin analyzeBuiltin(
        llvm::Function const &Builtin) const override {
      compiler::utils::NameMangler mangler(&Builtin.getParent()->getContext());
      llvm::StringRef BaseName = mangler.demangleName(Builtin.getName());

      if ((BaseName == "clmul") || (BaseName == "clmulh") ||
          (BaseName == "clmulr")) {
        unsigned Properties = compiler::utils::eBuiltinPropertyCanEmitInline |
                              compiler::utils::eBuiltinPropertyNoSideEffects;
        return compiler::utils::Builtin{
            Builtin, compiler::utils::eBuiltinUnknown,
            (compiler::utils::BuiltinProperties)Properties};
      }
      return compiler::utils::CLBuiltinInfo::analyzeBuiltin(Builtin);
    }
    llvm::Value *emitBuiltinInline(
        llvm::Function *Builtin, llvm::IRBuilder<> &B,
        llvm::ArrayRef<llvm::Value *> Args) override {
      if (Builtin) {       
        compiler::utils::NameMangler mangler(&Builtin->getParent()->getContext());
        llvm::StringRef BaseName = mangler.demangleName(Builtin->getName());           
        if (BaseName == "clmul") {
          return B.CreateIntrinsic(llvm::Intrinsic::riscv_clmul,
                                  Builtin->getReturnType(), {Args[0], Args[1]});
        } else if (BaseName == "clmulh") {
          return B.CreateIntrinsic(llvm::Intrinsic::riscv_clmulh,
                                  Builtin->getReturnType(), {Args[0], Args[1]});
        } else if (BaseName == "clmulr") {
          return B.CreateIntrinsic(llvm::Intrinsic::riscv_clmulr,
                                  Builtin->getReturnType(), {Args[0], Args[1]});
        }
      }
      return compiler::utils::CLBuiltinInfo::emitBuiltinInline(Builtin, B, Args);
    }
  };
}
{% endif -%}

namespace {{cookiecutter.target_name}} {
{{cookiecutter.target_name.capitalize()}}Module::{{cookiecutter.target_name.capitalize()}}Module({{cookiecutter.target_name.capitalize()}}Target &target, compiler::BaseContext &context,
               uint32_t &num_errors, std::string &log)
    : BaseModule(target, context, num_errors, log) {}

void {{cookiecutter.target_name.capitalize()}}Module::clear() {
  BaseModule::clear();
  object_code.clear();
}

compiler::Result {{cookiecutter.target_name.capitalize()}}Module::createBinary(cargo::array_view<std::uint8_t> &binary) {
  if (!finalized_llvm_module) {
    return compiler::Result::FINALIZE_PROGRAM_FAILURE;
  }

  // Lock the context, this is necessary due to analysis/pass managers being
  // owned by the LLVMContext and we are making heavy use of both below.
  std::lock_guard<compiler::BaseContext> contextLock(context);
  // Numerous things below touch LLVM's global state, in particular
  // retriggering command-line option parsing at various points. Ensure we
  // avoid data races by locking the LLVM global mutex.
  std::lock_guard<std::mutex> globalLock(compiler::utils::getLLVMGlobalMutex());

  // Write to an Elf object
  auto *TM = getTargetMachine();
  llvm::SmallVector<char, 512> objectBinary;
  llvm::raw_svector_ostream ostream(objectBinary);

  {
    compiler::Result err = compiler::Result::FAILURE;
    llvm::CrashRecoveryContext CRC;
    llvm::CrashRecoveryContext::Enable();
    bool crashed = !CRC.RunSafely([&] {
      err = compiler::emitCodeGenFile(*finalized_llvm_module, TM, ostream);
    });
    llvm::CrashRecoveryContext::Disable();
    if (crashed) {
      return compiler::Result::FINALIZE_PROGRAM_FAILURE;
    }
    if (compiler::Result::SUCCESS != err) {
      return err;
    }
  }

  llvm::ArrayRef<uint8_t> inputBinary{
      reinterpret_cast<uint8_t *>(objectBinary.data()),
      static_cast<std::size_t>(objectBinary.size())};

  llvm::SmallVector<std::string, 4> lld_args;
  // Set the entry point to the zero address to avoid a linker warning. The
  // entry point will not be used directly.
  lld_args.push_back("-e0");

  if ({{cookiecutter.link_shared}}) {
    lld_args.push_back("--shared");
  }

  {
    bool linkSuccess = false;
    llvm::CrashRecoveryContext CRC;
    llvm::CrashRecoveryContext::Enable();
    bool crashed = !CRC.RunSafely([&] {
      auto linkResult = compiler::utils::lldLinkToBinary(
          inputBinary, getTarget().hal_device_info->linker_script,
          getTarget().rt_lib, getTarget().rt_lib_size, lld_args);
      if (auto E = linkResult.takeError()) {
        std::string errStr = toString(std::move(E));
        addBuildError(errStr);
        if (auto callback = target.getNotifyCallbackFn()) {
          callback(errStr.c_str(), /*data*/ nullptr, /*data_size*/ 0);
        }
        return;
      }
      auto size = (*linkResult)->getBufferSize();
      if (cargo::success != object_code.alloc(size)) {
        return;
      }
      std::memcpy(object_code.data(), (*linkResult)->getBufferStart(), size);
      linkSuccess = true;
    });
    llvm::CrashRecoveryContext::Disable();
    if (crashed || !linkSuccess) {
      return compiler::Result::LINK_PROGRAM_FAILURE;
    }
  }

  // copy the generated ELF file to a specified path if desired
#if defined(CA_ENABLE_DEBUG_SUPPORT) || defined(CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE)
  if (!getTarget().env_debug_prefix.empty()) {
    std::string env_name = getTarget().env_debug_prefix + "_SAVE_ELF_PATH";
    if (const auto copyElfPath = llvm::sys::Process::GetEnv(env_name.c_str())) {
      llvm::SmallVector<char, 8> resultPath;
      std::error_code error;
      llvm::raw_fd_ostream of(*copyElfPath, error);
      if (error) {
        llvm::errs() << "Unable to open ELF file " << *copyElfPath << " :\n";
        llvm::errs() << "\t" << error.message() << "\n";
      } else {
        const uint8_t *elf_data = object_code.data();
        of.write(reinterpret_cast<const char *>(elf_data), object_code.size());
        llvm::errs() << "Writing ELF file  to " << *copyElfPath << "\n";
      }
    }
  }
#endif

  // return the binary buffer.
  binary = object_code;

  return compiler::Result::SUCCESS;
}

// No deferred support so just return nullptr
compiler::Kernel *{{cookiecutter.target_name.capitalize()}}Module::createKernel(const std::string &) { return nullptr; }

const {{cookiecutter.target_name.capitalize()}}Target &{{cookiecutter.target_name.capitalize()}}Module::getTarget() const {
  return *static_cast<{{cookiecutter.target_name.capitalize()}}Target *>(&target);
}

llvm::ModulePassManager {{cookiecutter.target_name.capitalize()}}Module::getLateTargetPasses(
    compiler::utils::PassMachinery &pass_mach) {
  if (getOptions().llvm_stats) {
    llvm::EnableStatistics();
  }

  return static_cast<{{cookiecutter.target_name.capitalize()}}PassMachinery &>(pass_mach).getLateTargetPasses();
}

static llvm::TargetMachine *createTargetMachine(const {{cookiecutter.target_name_capitalize}}Target &target) {
  // Init the llvm target machine.
  llvm::TargetOptions options;
  std::string error;
  const llvm::Target *llvm_target =
      llvm::TargetRegistry::lookupTarget(target.llvm_triple, error);
  if (nullptr == llvm_target) {
    return nullptr;
  }

  options.MCOptions.ABIName = target.llvm_abi;

  return llvm_target->createTargetMachine(
      target.llvm_triple, target.llvm_cpu, target.llvm_features, options,
      llvm::Reloc::Model::Static, llvm::CodeModel::Small,
      multi_llvm::CodeGenOptLevel::Aggressive);
}

llvm::TargetMachine *{{cookiecutter.target_name.capitalize()}}Module::getTargetMachine() {
  if (!target_machine.get()) {
    target_machine.reset(createTargetMachine(getTarget()));
  }
  return target_machine.get();
}
{% if "clmul"  in cookiecutter.feature.split(";") -%}
  std::unique_ptr<compiler::utils::BILangInfoConcept> createSimpleTargetCLBuiltinInfo(
      llvm::Module *Builtins) {
    return std::make_unique<CLTargetBuiltinInfo>(
        std::make_unique<compiler::utils::SimpleCLBuiltinLoader>(Builtins));
  }
{% endif -%}

std::unique_ptr<compiler::utils::PassMachinery> {{cookiecutter.target_name.capitalize()}}Module::createPassMachinery() {
  auto *TM = getTargetMachine();
  auto *Builtins = getTarget().getBuiltins();
  const auto &BaseContext = getTarget().getContext();

  compiler::utils::DeviceInfo Info = compiler::initDeviceInfoFromMux(
      getTarget().getCompilerInfo()->device_info);

  auto Callback = [Builtins](const llvm::Module &) {
{% if "clmul"  in cookiecutter.feature.split(";") -%}
    return compiler::utils::BuiltinInfo(createSimpleTargetCLBuiltinInfo(Builtins));
{% else -%}
    return compiler::utils::BuiltinInfo(compiler::utils::createCLBuiltinInfo(Builtins));
{% endif -%}
  };
  llvm::LLVMContext &Ctx = Builtins->getContext();
  return std::make_unique<{{cookiecutter.target_name.capitalize()}}PassMachinery>(
      getTarget(), Ctx, TM, Info, Callback,
      BaseContext.isLLVMVerifyEachEnabled(),
      BaseContext.getLLVMDebugLoggingLevel(),
      BaseContext.isLLVMTimePassesEnabled());
}

void {{cookiecutter.target_name.capitalize()}}Module::initializePassMachineryForFrontend(
    compiler::utils::PassMachinery &pass_mach,
    const clang::CodeGenOptions &CGO) const {
  // For historical reasons, loop interleaving is set to mirror setting for loop
  // unrolling. - comment from clang source
  llvm::PipelineTuningOptions PTO;
  PTO.LoopInterleaving = CGO.UnrollLoops;
  PTO.LoopVectorization = CGO.VectorizeLoop;
  PTO.SLPVectorization = CGO.VectorizeSLP;

  pass_mach.initializeStart(PTO);

  // Register the target library analysis directly and give it a customized
  // preset TLI.
  llvm::Triple TT = llvm::Triple(target_machine->getTargetTriple());
  auto TLII = llvm::TargetLibraryInfoImpl(TT);

  // getVecLib()'s return type changed in LLVM 18.
  auto VecLib = CGO.getVecLib();
  using VecLibT = decltype(VecLib);
  switch (VecLib) {
    case VecLibT::Accelerate:
      TLII.addVectorizableFunctionsFromVecLib(
          llvm::TargetLibraryInfoImpl::Accelerate, TT);
      break;
    case VecLibT::SVML:
      TLII.addVectorizableFunctionsFromVecLib(llvm::TargetLibraryInfoImpl::SVML,
                                              TT);
      break;
    case VecLibT::MASSV:
      TLII.addVectorizableFunctionsFromVecLib(
          llvm::TargetLibraryInfoImpl::MASSV, TT);
      break;
    case VecLibT::LIBMVEC:
      switch (TT.getArch()) {
        default:
          break;
        case llvm::Triple::x86_64:
          TLII.addVectorizableFunctionsFromVecLib(
              llvm::TargetLibraryInfoImpl::LIBMVEC_X86, TT);
          break;
      }
      break;
    default:
      break;
  }

  TLII.disableAllFunctions();

  pass_mach.getFAM().registerPass(
      [&TLII] { return llvm::TargetLibraryAnalysis(TLII); });
  auto *TM = pass_mach.getTM();
  if (TM) {
    pass_mach.getFAM().registerPass(
        [TM] { return llvm::TargetIRAnalysis(TM->getTargetIRAnalysis()); });
  }

  pass_mach.initializeFinish();
}

void {{cookiecutter.target_name.capitalize()}}Module::initializePassMachineryForFinalize(
    compiler::utils::PassMachinery &pass_mach) const {
  pass_mach.initializeStart();
  // Ensure that the optimizer doesn't inject calls to library functions that
  // can't be supported on a free-standing device.
  //
  // We cannot use PassManagerBuilder::LibraryInfo here, since the analysis
  // has to be added to the pass manager prior to other passes being added.
  // This is because other passes might require TargetLibraryInfoWrapper, and
  // if they do a TargetLibraryInfoImpl object with default settings will be
  // created prior to adding the pass. Trying to add a
  // TargetLibraryInfoWrapper analysis with disabled functions later will have
  // no affect, due to the analysis already being registered with the pass
  // manager.
  auto LibraryInfo =
      llvm::TargetLibraryInfoImpl(pass_mach.getTM()->getTargetTriple());
  LibraryInfo.disableAllFunctions();
  pass_mach.getFAM().registerPass(
      [&LibraryInfo] { return llvm::TargetLibraryAnalysis(LibraryInfo); });
  pass_mach.initializeFinish();
}

}  // namespace {{cookiecutter.target_name}}
