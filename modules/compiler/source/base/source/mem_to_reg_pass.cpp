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

#include <base/mem_to_reg_pass.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Transforms/Utils/PromoteMemToReg.h>
#include <multi_llvm/llvm_version.h>

using namespace llvm;

PreservedAnalyses compiler::MemToRegPass::run(Function &F,
                                              FunctionAnalysisManager &AM) {
  SmallVector<AllocaInst *, 16> allocasToPromote;

  for (auto &BB : F) {
    for (auto &I : BB) {
      if (auto *AI = dyn_cast<AllocaInst>(&I)) {
        if (isAllocaPromotable(AI)) {
          allocasToPromote.push_back(AI);
        }
      }
    }
  }

  PromoteMemToReg(allocasToPromote, AM.getResult<DominatorTreeAnalysis>(F));

  return allocasToPromote.empty() ? PreservedAnalyses::none()
                                  : PreservedAnalyses::all();
}
