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

#ifndef COMPILER_BASE_TARGET_H
#define COMPILER_BASE_TARGET_H

#include <cargo/optional.h>
#include <compiler/target.h>
#include <llvm/IR/DiagnosticInfo.h>
#include <llvm/IR/DiagnosticPrinter.h>
#include <mux/mux.h>

namespace compiler {
class BaseContext;

/// @brief Compiler target class.
class BaseTarget : public Target {
 public:
  BaseTarget(const compiler::Info *compiler_info, compiler::Context *context,
             compiler::NotifyCallbackFn callback);

  /// @brief Initialize the compiler target.
  ///
  /// @param[in] builtins_capabilities Capabilities for the embedded builtins.
  ///
  /// @return Return a status code.
  /// @retval `Result::SUCCESS` when initialization was successful.
  /// @retval `Result::INVALID_VALUE` if `builtins_capabilities` contains any
  /// invalid capabilities.
  /// @retval `Result::FAILURE` if any other failure occurred.
  Result init(uint32_t builtins_capabilities) override;

  /// @brief Returns the compiler info associated with this target.
  const compiler::Info *getCompilerInfo() const override final;

  compiler::BaseContext &getContext() const { return context; };

  llvm::Module *getBuiltins() const { return builtins.get(); };

  /// @see Target::listSnapshotStages
  compiler::Result listSnapshotStages(uint32_t count, const char **out_stages,
                                      uint32_t *out_count) override;

  /// @brief Provides derived targets with a way to supply additional "target"
  /// snapshots in addition to those supported by BaseTarget itself.
  std::vector<const char *> getTargetSnapshotStages() const;

  NotifyCallbackFn getNotifyCallbackFn() const { return callback; }

  /// @brief Returns the (non-null) LLVMContext.
  llvm::LLVMContext &getLLVMContext();
  /// @brief Returns the (non-null) LLVMContext.
  const llvm::LLVMContext &getLLVMContext() const;

 protected:
  /// @brief Initialize the compiler target after loading the builtins module.
  ///
  /// @param builtins an LLVM module containing the embedded builtins based on
  /// the capabilities provided to `init`.
  ///
  /// @return Return a status code.
  /// @retval `Result::SUCCESS` when initialization was successful.
  /// @retval `Result::FAILURE` if any other failure occurred.
  virtual Result initWithBuiltins(std::unique_ptr<llvm::Module> builtins) = 0;

  const compiler::Info *compiler_info;

  /// @brief LLVM context.
  llvm::LLVMContext llvm_context;

  /// @brief Context to use during initialization, and to pass to modules
  /// created with this target.
  compiler::BaseContext &context;

  /// @brief LLVM Module containing implementations of the builtin functions
  /// this target provides. May be null for compiler targets without external
  /// builtin libraries.
  std::unique_ptr<llvm::Module> builtins;

  /// @brief A list of additional target snapshots defined by derived
  /// implementations.
  std::vector<std::string> supported_target_snapshots;

  NotifyCallbackFn callback;
};
}  // namespace compiler

#endif  // COMPILER_BASE_TARGET_H
