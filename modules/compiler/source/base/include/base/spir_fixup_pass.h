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
/// @brief Fix SPIR IR pass.

#ifndef BASE_SPIR_FIXUP_PASS_H_INCLUDED
#define BASE_SPIR_FIXUP_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace spir {
/// @addtogroup cl_compiler
/// @{

/// @brief Pass to fix SPIR IR for the rest of compiler passes.
struct SpirFixupPass final : public llvm::PassInfoMixin<SpirFixupPass> {
  /// @brief Constructor.
  SpirFixupPass() = default;

  /// @brief The entry point to the SpirFixupPass.
  /// @param[in,out] module The module to run the pass on.
  /// @return Whether or not the pass changed anything in the module.
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};

/// @}
}  // namespace spir
}  // namespace compiler

#endif  // BASE_SPIR_FIXUP_PASS_H_INCLUDED
