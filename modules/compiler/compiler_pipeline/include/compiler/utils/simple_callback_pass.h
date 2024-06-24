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

/// @file
///
/// SimpleCallbackPass pass.

#ifndef COMPILER_UTILS_SIMPLE_CALLBACK_PASS_H_INCLUDED
#define COMPILER_UTILS_SIMPLE_CALLBACK_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>

#include <functional>

namespace compiler {
namespace utils {

using CallbackFnTy = void(llvm::Module &);

/// @brief Invokes a callback with the module when run.
/// Important: all analyses must be preserved by the callback function.
class SimpleCallbackPass : public llvm::PassInfoMixin<SimpleCallbackPass> {
 public:
  /// @param C Callback function to invoke when the pass is run.
  template <typename CallableT>
  SimpleCallbackPass(CallableT C) : Callback(std::move(C)) {}

  llvm::PreservedAnalyses run(llvm::Module &module,
                              llvm::ModuleAnalysisManager &) {
    Callback(module);
    return llvm::PreservedAnalyses::all();
  }

 private:
  std::function<CallbackFnTy> Callback;
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_SIMPLE_CALLBACK_PASS_H_INCLUDED
