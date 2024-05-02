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
/// Sub-group usage attribute pass.

#ifndef COMPILER_UTILS_SUB_GROUP_USAGE_PASS_H_INCLUDED
#define COMPILER_UTILS_SUB_GROUP_USAGE_PASS_H_INCLUDED

#include <compiler/utils/attributes.h>
#include <compiler/utils/sub_group_analysis.h>
#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief Sets (caches) function attributes concerning sub-group usage,
/// assuming they will not become invalidated by later passes.
class SubgroupUsagePass final : public llvm::PassInfoMixin<SubgroupUsagePass> {
 public:
  explicit SubgroupUsagePass() {}

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &AM) {
    const auto &GSGI = AM.getResult<SubgroupAnalysis>(M);

    for (auto &F : M) {
      if (!F.isDeclaration() && !GSGI.usesSubgroups(F)) {
        setHasNoExplicitSubgroups(F);
      }
    }
    return llvm::PreservedAnalyses::all();
  }
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_SUB_GROUP_USAGE_PASS_H_INCLUDED
