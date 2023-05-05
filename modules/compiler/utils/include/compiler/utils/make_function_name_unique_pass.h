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
/// Materialize missing builtins.

#ifndef COMPILER_UTILS_MAKE_FUNCTION_NAME_UNIQUE_PASS_H_INCLUDED
#define COMPILER_UTILS_MAKE_FUNCTION_NAME_UNIQUE_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief The MakeFunctionNameUnique pass changes the name of a specified
/// function
///
/// The module pass `MakeFunctionNameUniquePass` is used to give distinct names
/// to scheduled kernels. This is necessary since a single kernel can be run
/// more than once across different work sizes and we want to be able
/// differentiate them.
///
/// Runs over all kernels with "kernel entry point" metadata.
class MakeFunctionNameUniquePass final
    : public llvm::PassInfoMixin<MakeFunctionNameUniquePass> {
 public:
  /// @brief Constructor.
  ///
  /// @param Name An LLVM StringRef of the name to replace the function name
  /// with.
  MakeFunctionNameUniquePass(llvm::StringRef Name) : UniqueName(Name.str()) {}

  // This pass is not an optimization - it must be run
  static bool isRequired() { return true; }

  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);

 private:
  std::string UniqueName;
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_MAKE_FUNCTION_NAME_UNIQUE_PASS_H_INCLUDED
