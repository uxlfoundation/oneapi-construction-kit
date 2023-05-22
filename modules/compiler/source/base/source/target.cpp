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

#include <base/context.h>
#include <base/target.h>
#include <compiler/module.h>
#include <compiler/utils/memory_buffer.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/Module.h>

#include "bakery.h"

namespace compiler {
BaseTarget::BaseTarget(const compiler::Info *compiler_info,
                       compiler::Context *context, NotifyCallbackFn callback)
    : compiler_info(compiler_info),
      context(*static_cast<BaseContext *>(context)),
      callback{callback} {
  // Force enable opaque pointers.
  llvm_context.setOpaquePointers(true);

  if (callback) {
    auto diag_handler_callback_thunk = [](const llvm::DiagnosticInfo &DI,
                                          void *user_data) {
      if (auto *Remark =
              llvm::dyn_cast<llvm::DiagnosticInfoOptimizationBase>(&DI)) {
        if (!Remark->isEnabled()) {
          return;
        }
      }
      std::string log;
      llvm::raw_string_ostream stream{log};
      llvm::DiagnosticPrinterRawOStream diagnosticPrinter{stream};
      stream << llvm::LLVMContext::getDiagnosticMessagePrefix(DI.getSeverity())
             << ": ";
      DI.print(diagnosticPrinter);
      stream << "\n";
      auto *function = static_cast<NotifyCallbackFn *>(user_data);
      (*function)(log.c_str(), nullptr, 0);
    };
    void *notify_callback_fn_as_user_data =
        static_cast<void *>(&this->callback);
    getLLVMContext().setDiagnosticHandlerCallBack(
        diag_handler_callback_thunk, notify_callback_fn_as_user_data);
  }
}

Result BaseTarget::init(uint32_t builtins_capabilities) {
  const auto valid_capabilities_bitmask =
      BuiltinsCapabilities::CAPS_DEFAULT | BuiltinsCapabilities::CAPS_32BIT |
      BuiltinsCapabilities::CAPS_FP64 | BuiltinsCapabilities::CAPS_FP16;
  if (builtins_capabilities & ~valid_capabilities_bitmask) {
    return Result::INVALID_VALUE;
  }

  builtins::file::capabilities_bitfield caps = 0;
  if (builtins_capabilities & compiler::CAPS_32BIT) {
    caps |= builtins::file::CAPS_32BIT;
  }
  if (builtins_capabilities & compiler::CAPS_FP16) {
    caps |= builtins::file::CAPS_FP16;
  }
  if (builtins_capabilities & compiler::CAPS_FP64) {
    caps |= builtins::file::CAPS_FP64;
  }

  auto builtins_file = builtins::get_bc_file(caps);
  std::unique_ptr<llvm::Module> builtins_module_from_file = nullptr;

  if (builtins_file.data()) {
    auto error_or_builtins_module = llvm::getOwningLazyBitcodeModule(
        std::make_unique<compiler::utils::MemoryBuffer>(builtins_file.data(),
                                                        builtins_file.size()),
        getLLVMContext());
    if (!error_or_builtins_module) {
      return Result::FAILURE;
    }

    builtins_module_from_file = std::move(error_or_builtins_module.get());
    if ("unknown-unknown-unknown" !=
        builtins_module_from_file->getTargetTriple()) {
      return Result::FAILURE;
    }
  }

  return initWithBuiltins(std::move(builtins_module_from_file));
}

const compiler::Info *BaseTarget::getCompilerInfo() const {
  return compiler_info;
}

compiler::Result BaseTarget::listSnapshotStages(uint32_t count,
                                                const char **out_stages,
                                                uint32_t *out_count) {
  // BaseTargets support at least all "compiler" snapshot stages
  static std::array<const char *, 9> base_snapshot_stages = {
      compiler::SnapshotStage::COMPILE_DEFAULT,
      compiler::SnapshotStage::COMPILE_FRONTEND,
      compiler::SnapshotStage::COMPILE_LINKING,
      compiler::SnapshotStage::COMPILE_SIMD_PREP,
      compiler::SnapshotStage::COMPILE_SCALARIZED,
      compiler::SnapshotStage::COMPILE_LINEARIZED,
      compiler::SnapshotStage::COMPILE_SIMD_PACKETIZED,
      compiler::SnapshotStage::COMPILE_SPIR,
      compiler::SnapshotStage::COMPILE_BUILTINS,
  };

  // listSnapshotStages should fail if: out_stages is non-null and count is 0
  // or out stages is null and count is non-zero.
  if (!out_stages ^ (0 == count)) {
    return compiler::Result::INVALID_VALUE;
  }

  // Ask for any target-specific snapshot stages
  auto target_snapshot_stages = getTargetSnapshotStages();

  const uint32_t num_compiler_stages = base_snapshot_stages.size();

  if (out_stages) {
    // Only copy up to `count` number of stages from BaseModule.
    std::uninitialized_copy_n(base_snapshot_stages.begin(),
                              std::min(count, num_compiler_stages), out_stages);

    // We've filled out_stages[0] to out_stages[num_compiler_stages - 1], fill
    // the remaining stages, ensuring to break if `i` ends up going past `count`
    // or we run out of stages.
    auto iter = target_snapshot_stages.begin();
    for (uint32_t i = num_compiler_stages; i < count; ++i, ++iter) {
      // If we've run out of snapshot stages to populate.
      if (iter == target_snapshot_stages.end()) {
        break;
      }
      // FIXME: The lifetime of these stage names is subject to the target and
      // not under the user's control. See CA-4448.
      out_stages[i] = *iter;
    }
  }

  if (out_count) {
    *out_count = num_compiler_stages + target_snapshot_stages.size();
  }

  return compiler::Result::SUCCESS;
}

std::vector<const char *> BaseTarget::getTargetSnapshotStages() const {
  // The backing storage for the snapshot names is underspecified: see CA-4448.
  std::vector<const char *> snapshots(supported_target_snapshots.size());
  std::transform(supported_target_snapshots.begin(),
                 supported_target_snapshots.end(), snapshots.begin(),
                 [](const std::string &s) { return s.c_str(); });
  return snapshots;
}

/// @brief Returns the (non-null) LLVMContext.
llvm::LLVMContext &BaseTarget::getLLVMContext() { return llvm_context; }

/// @brief Returns the (non-null) LLVMContext.
const llvm::LLVMContext &BaseTarget::getLLVMContext() const {
  return llvm_context;
}
}  // namespace compiler
