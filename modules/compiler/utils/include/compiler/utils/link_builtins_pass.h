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
/// Link builtins pass.

#ifndef COMPILER_UTILS_LINK_BUILTINS_PASS_H_INCLUDED
#define COMPILER_UTILS_LINK_BUILTINS_PASS_H_INCLUDED

#include <compiler/utils/StructTypeRemapper.h>
#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {
/// @brief A pass for linking builtins to the current module
///
/// This pass will manually link in any functions required from a given
/// `builtins` module, into the current module.
class LinkBuiltinsPass final : public llvm::PassInfoMixin<LinkBuiltinsPass> {
 public:
  ///
  LinkBuiltinsPass() {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

 private:
  void cloneStructs(llvm::Module &M, llvm::Module &BuiltinsModule,
                    compiler::utils::StructMap &Map);
  void cloneBuiltins(
      llvm::Module &M, llvm::Module &BuiltinsModule,
      llvm::SmallVectorImpl<std::pair<llvm::Function *, bool>> &BuiltinFnDecls,
      compiler::utils::StructTypeRemapper *StructMap);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_LINK_BUILTINS_PASS_H_INCLUDED
