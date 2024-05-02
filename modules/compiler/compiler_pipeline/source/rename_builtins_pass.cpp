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

#include <compiler/utils/rename_builtins_pass.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>

llvm::PreservedAnalyses compiler::utils::RenameBuiltinsPass::run(
    llvm::Module &M, llvm::ModuleAnalysisManager &) {
  bool Changed = false;
  constexpr llvm::StringLiteral CoreFnPrefix = "__core";
  constexpr llvm::StringLiteral MuxFnPrefix = "__mux";

  for (auto &fn : M.functions()) {
    if (!fn.getName().starts_with(MuxFnPrefix)) {
      continue;
    }
    Changed = true;
    const llvm::StringRef OldFnName = fn.getName();
    const llvm::StringRef BaseFnName =
        OldFnName.substr(MuxFnPrefix.size(), OldFnName.size());
    const llvm::Twine NewFnName = CoreFnPrefix + BaseFnName;
    fn.setName(NewFnName);
  }
  return Changed ? llvm::PreservedAnalyses::all()
                 : llvm::PreservedAnalyses::none();
}
