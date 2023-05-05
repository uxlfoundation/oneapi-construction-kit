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
/// @brief Class ImageArgumentSubstitutionPass interface.

#ifndef BASE_IMAGE_ARGUMENT_SUBSTITUTION_PASS_H_INCLUDED
#define BASE_IMAGE_ARGUMENT_SUBSTITUTION_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
/// @addtogroup cl_compiler
/// @{

/// @brief Replace OpenCL image calls with those coming from image library.
struct ImageArgumentSubstitutionPass final
    : public llvm::PassInfoMixin<ImageArgumentSubstitutionPass> {
  /// @brief Replace OpenCL opaque image types with those from image library.
  ///
  /// Overrides llvm::ModulePass::runOnModule().
  ///
  /// @param[in,out] module The module to run the pass on.
  /// @param[in,out] am The analysis manager providing module analyses.
  ///
  /// @return Return the set of preserved analyses.
  llvm::PreservedAnalyses run(llvm::Module &module,
                              llvm::ModuleAnalysisManager &am);
};

/// @}
}  // namespace compiler

#endif  // BASE_IMAGE_ARGUMENT_SUBSTITUTION_PASS_H_INCLUDED
