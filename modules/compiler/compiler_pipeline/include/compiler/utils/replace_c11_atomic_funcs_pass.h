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
/// Replace C11 atomic functions pass.

#ifndef COMPILER_UTILS_REPLACE_C11_ATOMIC_FUNCS_PASS_H_INCLUDED
#define COMPILER_UTILS_REPLACE_C11_ATOMIC_FUNCS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass that changes C11 atomic functions to atomic instructions.
///
/// This pass replaces the subset of C11 atomic builtins we support with LLVM
/// instructions. Since we only support the minum requirements this pass does
/// not implement all the C11 atomic builtins e.g. atomic_(load|store) are not
/// implemented. Atomics are detected based on their mangled names, changes to
/// the name mangling ABI will cause this pass to break if it is not updated.
class ReplaceC11AtomicFuncsPass final
    : public llvm::PassInfoMixin<ReplaceC11AtomicFuncsPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REPLACE_C11_ATOMIC_FUNCS_PASS_H_INCLUDED
