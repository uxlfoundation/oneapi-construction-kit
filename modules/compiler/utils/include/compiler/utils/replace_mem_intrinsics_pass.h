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
/// A pass that replaces calls to llvm.memcpy.*, llvm.memset.* and
/// llvm.memmove.* with calls to a generated loop.

#ifndef COMPILER_UTILS_REPLACE_MEM_INTRINSICS_PASS_H_INCLUDED
#define COMPILER_UTILS_REPLACE_MEM_INTRINSICS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass that replaces calls to llvm.memcpy.*, llvm.memset.* and
/// llvm.memmove with calls to a generated loop.
/// @note This pass can be used for targets which are not able to generate
/// backend code for these intrinsics or do not link with a library which
/// supports this. Note that memove only supports same address spaces for the
/// pointer arguments
class ReplaceMemIntrinsicsPass final
    : public llvm::PassInfoMixin<ReplaceMemIntrinsicsPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REPLACE_MEM_INTRINSICS_PASS_H_INCLUDED
