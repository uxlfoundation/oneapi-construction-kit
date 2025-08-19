// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
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
#include <llvm/IR/Module.h>
#include <mux/mux.h>

#include <optional>

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

  virtual llvm::Module *getBuiltins() const = 0;

  NotifyCallbackFn getNotifyCallbackFn() const { return callback; }

  /// @brief Calls a function with the LLVMContext, taking into account any
  /// required locking to allow the function exclusive use.
  virtual void withLLVMContextDo(void (*)(llvm::LLVMContext &, void *),
                                 void *) = 0;

  /// @brief Calls a function with the (non-null) LLVMContext, taking into
  /// account any required locking to allow the function exclusive use.
  template <typename F>
  auto withLLVMContextDo(F &&f) -> std::enable_if_t<std::is_void_v<
      decltype(std::forward<F>(f)(std::declval<llvm::LLVMContext &>()))>> {
    withLLVMContextDo(
        [](llvm::LLVMContext &C, void *f) {
          (std::forward<F>(*static_cast<F *>(f)))(C);
        },
        std::addressof(f));
  }

  /// @brief Calls a function with the (non-null) LLVMContext, taking into
  /// account any required locking to allow the function exclusive use.
  template <typename F>
  auto withLLVMContextDo(F &&f) -> std::enable_if_t<
      std::is_object_v<
          decltype(std::forward<F>(f)(std::declval<llvm::LLVMContext &>()))>,
      decltype(std::forward<F>(f)(std::declval<llvm::LLVMContext &>()))> {
    using ResultTy =
        decltype(std::forward<F>(f)(std::declval<llvm::LLVMContext &>()));
    // Ideally this would use std::promise to merge the value and reference
    // implementations, but older versions of MSVC do not accept
    // non-default-constructible types for that; work around it by using
    // std::optional instead.
    std::optional<ResultTy> result;
    withLLVMContextDo(
        [&](llvm::LLVMContext &C) { result = std::forward<F>(f)(C); });
    assert(result.has_value() && "result should have been assigned");
    return std::move(result.value());
  }

  /// @brief Calls a function with the (non-null) LLVMContext, taking into
  /// account any required locking to allow the function exclusive use.
  template <typename F>
  auto withLLVMContextDo(F &&f) -> std::enable_if_t<
      std::is_reference_v<
          decltype(std::forward<F>(f)(std::declval<llvm::LLVMContext &>()))>,
      decltype(std::forward<F>(f)(std::declval<llvm::LLVMContext &>()))> {
    using ResultTy =
        decltype(std::forward<F>(f)(std::declval<llvm::LLVMContext &>()));
    // std::optional does not accept references. Use a pointer instead.
    std::optional<std::remove_reference_t<ResultTy> *> resultPtr;
    withLLVMContextDo([&](llvm::LLVMContext &C) {
      auto &&resultRef = std::forward<F>(f)(C);
      resultPtr = std::addressof(resultRef);
    });
    return static_cast<ResultTy>(*resultPtr.value());
  }

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

  /// @brief Context to use during initialization, and to pass to modules
  /// created with this target.
  compiler::BaseContext &context;

  NotifyCallbackFn callback;
};

/// @brief A utility class for an ahead-of-time compilation target.
///
/// This target owns the LLVMContext and dependent LLVM resources like the
/// builtins module, if used.
class BaseAOTTarget : public BaseTarget {
 public:
  BaseAOTTarget(const compiler::Info *compiler_info, compiler::Context *context,
                NotifyCallbackFn callback);

  /// @see BaseTarget::withLLVMContextDo
  void withLLVMContextDo(void (*)(llvm::LLVMContext &, void *),
                         void *) override;

  /// @see BaseTarget::getBuiltins
  llvm::Module *getBuiltins() const override { return builtins.get(); };

 protected:
  /// @brief LLVM context.
  llvm::LLVMContext llvm_context;

  /// @brief Mutex for accessing the LLVM context.
  std::mutex llvm_context_mutex;

  /// @brief LLVM Module containing implementations of the builtin functions
  /// this target provides. May be null for compiler targets without external
  /// builtin libraries.
  std::unique_ptr<llvm::Module> builtins;
};

}  // namespace compiler

#endif  // COMPILER_BASE_TARGET_H
