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
#include <multi_llvm/llvm_version.h>

#include "bakery.h"

namespace compiler {
BaseTarget::BaseTarget(const compiler::Info *compiler_info,
                       compiler::Context *context, NotifyCallbackFn callback)
    : compiler_info(compiler_info),
      context(*static_cast<BaseContext *>(context)),
      callback{callback} {}

Result BaseTarget::init(uint32_t builtins_capabilities) {
#if LLVM_VERSION_LESS(17, 0)
  getLLVMContext().setOpaquePointers(true);
#endif
  if (callback) {
    auto diag_handler_callback_thunk = [](
#if LLVM_VERSION_GREATER_EQUAL(19, 0)
                                           const llvm::DiagnosticInfo *DI,
#else
                                           const llvm::DiagnosticInfo &DI_,
#endif
                                           void *user_data) {
#if LLVM_VERSION_LESS(19, 0)
      const llvm::DiagnosticInfo *DI = &DI_;
#endif
      if (auto *Remark =
              llvm::dyn_cast<llvm::DiagnosticInfoOptimizationBase>(DI)) {
        if (!Remark->isEnabled()) {
          return;
        }
      }
      std::string log;
      llvm::raw_string_ostream stream{log};
      llvm::DiagnosticPrinterRawOStream diagnosticPrinter{stream};
      stream << llvm::LLVMContext::getDiagnosticMessagePrefix(DI->getSeverity())
             << ": ";
      DI->print(diagnosticPrinter);
      stream << "\n";
      auto *function = static_cast<NotifyCallbackFn *>(user_data);
      (*function)(log.c_str(), nullptr, 0);
    };
    void *notify_callback_fn_as_user_data =
        static_cast<void *>(&this->callback);
    getLLVMContext().setDiagnosticHandlerCallBack(
        diag_handler_callback_thunk, notify_callback_fn_as_user_data);
  }

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

#if LLVM_VERSION_GREATER_EQUAL(19, 0)
    if (UseNewDbgInfoFormat && !builtins_module_from_file->IsNewDbgInfoFormat) {
      builtins_module_from_file->convertToNewDbgValues();
    }
#endif
  }

  return initWithBuiltins(std::move(builtins_module_from_file));
}

const compiler::Info *BaseTarget::getCompilerInfo() const {
  return compiler_info;
}

BaseAOTTarget::BaseAOTTarget(const compiler::Info *compiler_info,
                             compiler::Context *context,
                             NotifyCallbackFn callback)
    : BaseTarget(compiler_info, context, callback) {}

llvm::LLVMContext &BaseAOTTarget::getLLVMContext() { return llvm_context; }

const llvm::LLVMContext &BaseAOTTarget::getLLVMContext() const {
  return llvm_context;
}

}  // namespace compiler
