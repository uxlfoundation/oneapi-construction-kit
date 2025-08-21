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
/// @brief Simplify OpenCL builtins pass.

#ifndef BASE_MEM_TO_REG_PASS_H_INCLUDED
#define BASE_MEM_TO_REG_PASS_H_INCLUDED

#include <llvm/IR/Dominators.h>
#include <llvm/IR/PassManager.h>

namespace compiler {
/// @addtogroup cl_compiler
/// @{

/// @brief Codeplay's mem to reg pass.
struct MemToRegPass final : public llvm::PassInfoMixin<MemToRegPass> {
  /// @brief The entry point to the MemToRegPass.
  ///
  /// @param[in,out] function The function to run the pass on.
  /// @param[in,out] am Analysis manager providing analyses.
  /// @return Whether or not the pass changed anything in the function.
  llvm::PreservedAnalyses run(llvm::Function &function,
                              llvm::FunctionAnalysisManager &am);
};

/// @}
} // namespace compiler

#endif // BASE_MEM_TO_REG_PASS_H_INCLUDED
