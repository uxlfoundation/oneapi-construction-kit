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

#include <base/base_module_pass_machinery.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/cl_builtin_info.h>
#include <compiler/utils/encode_kernel_metadata_pass.h>
#include <compiler/utils/llvm_global_mutex.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/metadata_analysis.h>
#include <compiler/utils/pass_functions.h>
#include <host/compiler_kernel.h>
#include <host/host_mux_builtin_info.h>
#include <host/host_pass_machinery.h>
#include <host/module.h>
#include <host/target.h>
#include <host/utils/relocations.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CrashRecoveryContext.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <multi_llvm/llvm_version.h>

#include "cargo/expected.h"
#include "tracer/tracer.h"

namespace host {

HostKernel::HostKernel(HostTarget &target, compiler::Options &build_options,
                       llvm::Module *module, std::string name,
                       std::array<size_t, 3> preferred_local_sizes,
                       size_t local_memory_used)
    : BaseKernel(name, preferred_local_sizes[0], preferred_local_sizes[1],
                 preferred_local_sizes[2], local_memory_used),
      module(module),
      target(target),
      build_options(build_options) {}

HostKernel::~HostKernel() {
  if (target.orc_engine) {
    auto &es = target.orc_engine->getExecutionSession();
    for (const auto &name : kernel_jit_dylibs) {
      if (auto *jit = es.getJITDylibByName(name)) {
        llvm::cantFail(es.removeJITDylib(*jit));
      }
    }
  }
}

compiler::Result HostKernel::precacheLocalSize(size_t local_size_x,
                                               size_t local_size_y,
                                               size_t local_size_z) {
  if (local_size_x == 0 || local_size_y == 0 || local_size_z == 0) {
    return compiler::Result::INVALID_VALUE;
  }

  auto optimized_kernel =
      lookupOrCreateOptimizedKernel({local_size_x, local_size_y, local_size_z});
  if (!optimized_kernel) {
    return optimized_kernel.error();
  }
  return compiler::Result::SUCCESS;
}

cargo::expected<uint32_t, compiler::Result> HostKernel::getDynamicWorkWidth(
    size_t local_size_x, size_t local_size_y, size_t local_size_z) {
  auto optimized_kernel =
      lookupOrCreateOptimizedKernel({local_size_x, local_size_y, local_size_z});
  if (!optimized_kernel) {
    return cargo::make_unexpected(optimized_kernel.error());
  }
  // We report the preferred work width as the maximum work width.
  return optimized_kernel->binary_kernel->pref_work_width;
}

cargo::expected<cargo::dynamic_array<uint8_t>, compiler::Result>
HostKernel::createSpecializedKernel(
    const mux_ndrange_options_t &specialization_options) {
  if (!specialization_options.descriptors &&
      specialization_options.descriptors_length > 0) {
    return cargo::make_unexpected(compiler::Result::INVALID_VALUE);
  }

  if (specialization_options.descriptors &&
      specialization_options.descriptors_length == 0) {
    return cargo::make_unexpected(compiler::Result::INVALID_VALUE);
  }

  for (int i = 0; i < 3; i++) {
    if (specialization_options.local_size[i] == 0) {
      return cargo::make_unexpected(compiler::Result::INVALID_VALUE);
    }
  }

  if (!specialization_options.global_offset) {
    return cargo::make_unexpected(compiler::Result::INVALID_VALUE);
  }

  if (!specialization_options.global_size) {
    return cargo::make_unexpected(compiler::Result::INVALID_VALUE);
  }

  if (specialization_options.dimensions == 0 ||
      specialization_options.dimensions > 3) {
    return cargo::make_unexpected(compiler::Result::INVALID_VALUE);
  }

  if (target.getCompilerInfo()->device_info->custom_buffer_capabilities == 0) {
    for (uint64_t i = 0; i < specialization_options.descriptors_length; i++) {
      if (specialization_options.descriptors[i].type ==
          mux_descriptor_info_type_custom_buffer) {
        return cargo::make_unexpected(compiler::Result::INVALID_VALUE);
      }
    }
  }

  std::array<size_t, 3> local_size;
  std::copy(std::begin(specialization_options.local_size),
            std::end(specialization_options.local_size),
            std::begin(local_size));
  auto optimized_kernel = lookupOrCreateOptimizedKernel(local_size);
  if (!optimized_kernel) {
    return cargo::make_unexpected(optimized_kernel.error());
  }

  cargo::dynamic_array<uint8_t> binary_out;
  if (binary_out.alloc(host::utils::getSizeForJITKernel())) {
    return cargo::make_unexpected(compiler::Result::OUT_OF_MEMORY);
  }
  host::utils::serializeJITKernel(optimized_kernel->binary_kernel.get(),
                                  binary_out.data());
  return {std::move(binary_out)};
}

cargo::expected<uint32_t, compiler::Result>
HostKernel::querySubGroupSizeForLocalSize(size_t local_size_x,
                                          size_t local_size_y,
                                          size_t local_size_z) {
  auto optimized_kernel =
      lookupOrCreateOptimizedKernel({local_size_x, local_size_y, local_size_z});
  if (!optimized_kernel) {
    return cargo::make_unexpected(optimized_kernel.error());
  }
  // If we've compiled with degenerate sub-groups, the sub-group size is the
  // work-group size.
  if (optimized_kernel->binary_kernel->sub_group_size == 0) {
    return local_size_x * local_size_y * local_size_z;
  }

  // Otherwise, on host we always use vectorize in the x-dimension, so
  // sub-groups "go" in the x-dimension.
  return std::min(
      local_size_x,
      static_cast<size_t>(optimized_kernel->binary_kernel->sub_group_size));
}

cargo::expected<std::array<size_t, 3>, compiler::Result>
HostKernel::queryLocalSizeForSubGroupCount(size_t sub_group_count) {
  // Try to compile something and see what subgroup size we get
  const auto &info = *target.getCompilerInfo()->device_info;
  const size_t max_local_size_x = info.max_work_group_size_x;
  auto optimized_kernel =
      lookupOrCreateOptimizedKernel({max_local_size_x, 1, 1});
  if (!optimized_kernel) {
    return cargo::make_unexpected(optimized_kernel.error());
  }

  // If we've compiled with degenerate sub-groups, the work-group size is the
  // sub-group size.
  const auto sub_group_size = optimized_kernel->binary_kernel->sub_group_size;
  if (sub_group_size == 0) {
    // FIXME: For degenerate sub-groups, the local size could be anything up to
    // the maximum local size. For any other sub-group count, we should ensure
    // that the work-group size we report comes back through the deferred
    // kernel's sub-group count when it comes to compiling it. See CA-4784.
    if (sub_group_count == 1) {
      return {{max_local_size_x, 1, 1}};
    } else {
      // If we asked for anything other than a single subgroup, but we have got
      // degenerate subgroups, then we are in some amount of trouble.
      return {{0, 0, 0}};
    }
  }

  const auto local_size = sub_group_count * sub_group_size;
  if (local_size <= max_local_size_x) {
    return {{local_size, 1, 1}};
  }

  return {{0, 0, 0}};
};

cargo::expected<size_t, compiler::Result> HostKernel::queryMaxSubGroupCount() {
  // Without compiling this kernel, we can't determine the actual maximum
  // number of sub-groups, and we can't meaningfully compile unless we know the
  // local size.
  // Our implementation allows the compiler to generate multiple variants of a
  // kernel with different sub-group sizes, and choose to dispatch each
  // depending on what suits the ND range. The OpenCL and SYCL specifications
  // are loose enough to permit this too.
  // So, we return the device-specific maximum number of sub-groups, assuming
  // that this kernel *could* be compiled with a trivial sub-group size of 1
  // for a given ND-range.
  const auto &info = *target.getCompilerInfo()->device_info;
  return static_cast<size_t>(info.max_sub_group_count);
}

cargo::expected<const OptimizedKernel &, compiler::Result>
HostKernel::lookupOrCreateOptimizedKernel(std::array<size_t, 3> local_size) {
  if (0 < optimized_kernel_map.count(local_size)) {
    return optimized_kernel_map[local_size];
  }

  {
    const std::lock_guard<compiler::Context> guard(target.getContext());

    std::unique_ptr<llvm::Module> optimized_module(llvm::CloneModule(*module));
    if (nullptr == optimized_module) {
      return cargo::make_unexpected(compiler::Result::OUT_OF_MEMORY);
    }

    // max length of a uint64_t is 20 digits, 64 just to be comfortable with the
    // prefix of '__mux_host_'
    const unsigned unique_name_data_length = 64;
    char unique_name_data[unique_name_data_length];
    if (snprintf(unique_name_data, unique_name_data_length,
                 "__mux_host_%" PRIu64, target.unique_identifier++) < 0) {
      return cargo::make_unexpected(compiler::Result::FAILURE);
    }
    std::string unique_name(unique_name_data);

    auto device_info = target.getCompilerInfo()->device_info;

    // FIXME: Ideally we'd be able to call/reuse HostModule::createPassMachinery
    // but we only have access to the HostTarget
    auto *const TM = target.target_machine.get();
    auto builtinInfoCallback = [&](const llvm::Module &) {
      return compiler::utils::BuiltinInfo(
          std::make_unique<HostBIMuxInfo>(),
          compiler::utils::createCLBuiltinInfo(target.getBuiltins()));
    };
    auto deviceInfo = compiler::initDeviceInfoFromMux(device_info);
    HostPassMachinery pass_mach(module->getContext(), TM, deviceInfo,
                                builtinInfoCallback,
                                target.getContext().isLLVMVerifyEachEnabled(),
                                target.getContext().getLLVMDebugLoggingLevel(),
                                target.getContext().isLLVMTimePassesEnabled());
    pass_mach.setCompilerOptions(build_options);
    host::initializePassMachineryForFinalize(pass_mach, target);

    llvm::ModulePassManager pm;
    // Set up the kernel metadata which informs later passes which kernel we're
    // interested in optimizing. We've already done this when initially
    // creating the kernel, but now we have more accurate local size data.
    compiler::utils::EncodeKernelMetadataPassOptions pass_opts;
    pass_opts.KernelName = name;
    pass_opts.LocalSizes = {static_cast<uint64_t>(local_size[0]),
                            static_cast<uint64_t>(local_size[1]),
                            static_cast<uint64_t>(local_size[2])};
    pm.addPass(compiler::utils::EncodeKernelMetadataPass(pass_opts));

    pm.addPass(pass_mach.getKernelFinalizationPasses(unique_name));

    {
      // Using the CrashRecoveryContext and statistics touches LLVM's global
      // state.
      const std::lock_guard<std::mutex> globalLock(
          compiler::utils::getLLVMGlobalMutex());
      llvm::CrashRecoveryContext CRC;
      llvm::CrashRecoveryContext::Enable();
      const bool crashed = !CRC.RunSafely(
          [&] { pm.run(*optimized_module, pass_mach.getMAM()); });
      llvm::CrashRecoveryContext::Disable();
      if (crashed) {
        return cargo::make_unexpected(
            compiler::Result::FINALIZE_PROGRAM_FAILURE);
      }

      if (llvm::AreStatisticsEnabled()) {
        llvm::PrintStatistics();
      }
    }

    // Retrieve the vectorization width and amount of local memory used.
    auto default_work_width = FixedOrScalableQuantity<uint32_t>::getOne();
    handler::VectorizeInfoMetadata fn_metadata(
        unique_name, unique_name,
        /* local_memory_usage */ 0,
        /* sub_group_size */ FixedOrScalableQuantity<uint32_t>(),
        /* min_work_item_factor= */ default_work_width,
        /* pref_work_item_factor */ default_work_width);
    if (auto *f = optimized_module->getFunction(unique_name)) {
      fn_metadata =
          pass_mach.getFAM()
              .getResult<compiler::utils::VectorizeMetadataAnalysis>(*f);
    }

    // Host doesn't support scalable values.
    if (fn_metadata.min_work_item_factor.isScalable() ||
        fn_metadata.pref_work_item_factor.isScalable() ||
        fn_metadata.sub_group_size.isScalable()) {
      return cargo::make_unexpected(compiler::Result::FINALIZE_PROGRAM_FAILURE);
    }

    // Note that we grab a handle to the module here, which we use to reference
    // the module going forward. This is despite us passing ownership of the
    // module off to the JITDylib. As long as the JITDylib outlives all uses of
    // the optimized kernels, this should be okay; the JIT has the same lifetime
    // as this HostKernel.
    llvm::Module *optimized_module_ptr = optimized_module.get();

    // Create a unique JITDylib for this instance of the kernel, so that its
    // symbols don't clash with any other kernel's symbols.
    auto jd = target.orc_engine->createJITDylib(unique_name + ".dylib");
    if (auto err = jd.takeError()) {
      if (auto callback = target.getNotifyCallbackFn()) {
        callback(llvm::toString(std::move(err)).c_str(), /*data*/ nullptr,
                 /*data_size*/ 0);
      } else {
        llvm::consumeError(std::move(err));
      }
      return cargo::make_unexpected(compiler::Result::FINALIZE_PROGRAM_FAILURE);
    }
    // Register this JITDylib so we can clear up its resources later.
    kernel_jit_dylibs.insert(jd->getName());

    llvm::orc::SymbolMap symbols;
    llvm::orc::MangleAndInterner mangle(
        target.orc_engine->getExecutionSession(),
        target.orc_engine->getDataLayout());

    for (const auto &reloc : host::utils::getRelocations()) {
#if LLVM_VERSION_GREATER_EQUAL(17, 0)
      symbols[mangle(reloc.first)] = {llvm::orc::ExecutorAddr(reloc.second),
                                      llvm::JITSymbolFlags::Exported};
#else
      symbols[mangle(reloc.first)] = llvm::JITEvaluatedSymbol(
          reloc.second, llvm::JITSymbolFlags::Exported);
#endif
    }

    // Define our runtime library symbols required for the JIT to successfully
    // link.
    if (auto err = jd->define(llvm::orc::absoluteSymbols(std::move(symbols)))) {
      if (auto callback = target.getNotifyCallbackFn()) {
        callback(llvm::toString(std::move(err)).c_str(), /*data*/ nullptr,
                 /*data_size*/ 0);
      } else {
        llvm::consumeError(std::move(err));
      }
      return cargo::make_unexpected(compiler::Result::FINALIZE_PROGRAM_FAILURE);
    }

    // Add the module.
    if (auto err = target.orc_engine->addIRModule(
            *jd, llvm::orc::ThreadSafeModule(std::move(optimized_module),
                                             target.llvm_ts_context))) {
      if (auto callback = target.getNotifyCallbackFn()) {
        callback(llvm::toString(std::move(err)).c_str(), /*data*/ nullptr,
                 /*data_size*/ 0);
      } else {
        llvm::consumeError(std::move(err));
      }
      return cargo::make_unexpected(compiler::Result::FINALIZE_PROGRAM_FAILURE);
    }

    // Retrieve the kernel address.
    uint64_t hook;
    {
      // Compiling the kernel may touch the global LLVM state
      const std::lock_guard<std::mutex> globalLock(
          compiler::utils::getLLVMGlobalMutex());

      // We cannot safely look up any symbol inside a CrashRecoveryContext
      // because the CRC handles errors by a longjmp back to safety, skipping
      // over destructors of objects that do need to be destroyed. We do so
      // anyway because the effect is less bad than crashing right away.
      std::promise<uint64_t> promise;
      llvm::Error err = llvm::Error::success();
      llvm::cantFail(std::move(err));

      auto &es = target.orc_engine->getExecutionSession();
      auto so = makeJITDylibSearchOrder(
          &*jd, llvm::orc::JITDylibLookupFlags::MatchAllSymbols);
      auto name = target.orc_engine->mangleAndIntern(unique_name);
      llvm::orc::SymbolLookupSet names({name});
      llvm::orc::SymbolsResolvedCallback notifyComplete =
          [&](llvm::Expected<llvm::orc::SymbolMap> r) {
            if (r) {
              assert(r->size() == 1 && "Unexpected number of results");
              assert(r->count(name) && "Missing result for symbol");
              auto address = r->begin()->second.getAddress();
#if LLVM_VERSION_GREATER_EQUAL(17, 0)
              promise.set_value(address.getValue());
#else
              promise.set_value(address);
#endif
            } else {
              const llvm::ErrorAsOutParameter _(&err);
              err = r.takeError();
              promise.set_value(0);
            }
          };

      bool crashed;
      {
        llvm::CrashRecoveryContext crc;
        llvm::CrashRecoveryContext::Enable();
        crashed = !crc.RunSafely([&] {
          es.lookup(llvm::orc::LookupKind::Static, std::move(so),
                    std::move(names), llvm::orc::SymbolState::Ready,
                    std::move(notifyComplete),
                    llvm::orc::NoDependenciesToRegister);
          hook = promise.get_future().get();
        });
        llvm::CrashRecoveryContext::Disable();
      }

      if (crashed) {
        // If we crashed, remove the dylib now so that the lookup callback
        // runs right away and does not try to access the promise after it has
        // already been destroyed. Note that this guarantees err will be set and
        // we return an error.
        llvm::cantFail(es.removeJITDylib(*jd));
        promise.get_future().get();
      }

      if (err) {
        if (auto callback = target.getNotifyCallbackFn()) {
          callback(llvm::toString(std::move(err)).c_str(), /*data*/ nullptr,
                   /*data_size*/ 0);
        } else {
          llvm::consumeError(std::move(err));
        }
        return cargo::make_unexpected(
            compiler::Result::FINALIZE_PROGRAM_FAILURE);
      }
    }

    const uint32_t min_width = fn_metadata.min_work_item_factor.getFixedValue();
    const uint32_t pref_width =
        fn_metadata.pref_work_item_factor.getFixedValue();
    const uint32_t sub_group_size = fn_metadata.sub_group_size.getFixedValue();

    std::unique_ptr<host::utils::jit_kernel_s> jit_kernel(
        new host::utils::jit_kernel_s{
            name, hook, static_cast<uint32_t>(fn_metadata.local_memory_usage),
            min_width, pref_width, sub_group_size});
    optimized_kernel_map.emplace(
        local_size,
        OptimizedKernel{optimized_module_ptr, std::move(jit_kernel)});
  }
  return optimized_kernel_map[local_size];
}
}  // namespace host
