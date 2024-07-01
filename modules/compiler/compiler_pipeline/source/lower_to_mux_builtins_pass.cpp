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

#include <compiler/utils/builtin_info.h>
#include <compiler/utils/lower_to_mux_builtins_pass.h>
#include <llvm/IR/Module.h>

using namespace llvm;

PreservedAnalyses compiler::utils::LowerToMuxBuiltinsPass::run(
    Module &M, ModuleAnalysisManager &AM) {
  auto &BI = AM.getResult<BuiltinInfoAnalysis>(M);

  SmallVector<CallInst *, 8> Calls;
  for (auto &F : M.functions()) {
    auto B = BI.analyzeBuiltin(F);
    if (B.properties & eBuiltinPropertyLowerToMuxBuiltin) {
      for (auto *U : F.users()) {
        if (auto *CI = dyn_cast<CallInst>(U)) {
          Calls.push_back(CI);
        }
      }
    }
  }

  if (Calls.empty()) {
    return PreservedAnalyses::all();
  }

  for (auto *CI : Calls) {
    if (auto *const NewCI = BI.lowerBuiltinToMuxBuiltin(*CI)) {
      CI->replaceAllUsesWith(NewCI);
      CI->eraseFromParent();
    }
  }

  return PreservedAnalyses::none();
}
