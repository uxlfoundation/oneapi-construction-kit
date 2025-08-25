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

/// @file
///
/// Remove address spaces pass.

#ifndef COMPILER_UTILS_REMOVE_ADDRESS_SPACES_PASS_H_INCLUDED
#define COMPILER_UTILS_REMOVE_ADDRESS_SPACES_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>
#include <multi_llvm/multi_llvm.h>

#if LLVM_VERSION_LESS(20, 0)
namespace compiler {
namespace utils {

/// @brief A pass that removes address spaces from functions to work around LLVM
/// issue #124759.
class RemoveAddressSpacesPass final
    : public llvm::PassInfoMixin<RemoveAddressSpacesPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler
#endif

#endif  // COMPILER_UTILS_REMOVE_ADDRESS_SPACES_PASS_H_INCLUDED
