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
/// Replace address space qualifier functions pass

#ifndef UTILS_REPLACE_ADDRESS_SPACE_QUALIFIER_FUNCTIONS_PASS_H_INCLUDED
#define UTILS_REPLACE_ADDRESS_SPACE_QUALIFIER_FUNCTIONS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass that turns address space qualifier conversion functions into
/// address space casts, for targets that have only a unified memory layout.
///
class ReplaceAddressSpaceQualifierFunctionsPass final
    : public llvm::PassInfoMixin<ReplaceAddressSpaceQualifierFunctionsPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

}  // namespace utils
}  // namespace compiler

#endif  // UTILS_REPLACE_ADDRESS_SPACE_QUALIFIER_FUNCTIONS_PASS_H_INCLUDED
