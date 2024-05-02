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
/// Remove llvm lifetime intrinsics pass

#ifndef COMPILER_UTILS_REMOVE_LIFETIME_INTRINSICS_PASS_H_INCLUDED
#define COMPILER_UTILS_REMOVE_LIFETIME_INTRINSICS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass for removing lifetime intrinsics from all instructions in a
/// module
///
/// The function pass `RemoveLifetimeIntrinsics` removes the
/// `llvm.lifetime.start` and `llvm.lifetime.end` intrinsics from a module by
/// iterating over all the instructions and erasing any lifetime intrinsics
/// found, as well as the bitcasts they use for the pointer argument. Removing
/// this information is useful for debugging since the backend is less likely to
/// optimize away variables in the stack no longer used, as a result this pass
/// should only be run on debug builds of the module.
class RemoveLifetimeIntrinsicsPass final
    : public llvm::PassInfoMixin<RemoveLifetimeIntrinsicsPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REMOVE_LIFETIME_INTRINSICS_PASS_H_INCLUDED
