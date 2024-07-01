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

#include <base/set_convergent_attr_pass.h>
#include <compiler/utils/builtin_info.h>
#include <llvm/ADT/PriorityWorklist.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

#define DEBUG_TYPE "set-convergent-attr"

using namespace llvm;

PreservedAnalyses compiler::SetConvergentAttrPass::run(
    Module &M, ModuleAnalysisManager &AM) {
  SmallPtrSet<Function *, 4> ConvergentFns;
  SmallPriorityWorklist<Function *, 4> Worklist;

  auto &BI = AM.getResult<utils::BuiltinInfoAnalysis>(M);

  // Collect the leaf functions which are to be marked convergent
  for (auto &F : M.functions()) {
    if (F.isConvergent()) {
      Worklist.insert(&F);
      ConvergentFns.insert(&F);
    } else if (F.isDeclaration() && !F.isIntrinsic()) {
      // Only check declarations for convergence; assume that any bodies have
      // been marked correctly already (above) or call to convergent
      // declarations (below).
      auto B = BI.analyzeBuiltin(F);
      if (!(B.properties & utils::eBuiltinPropertyKnownNonConvergent)) {
        Worklist.insert(&F);
        ConvergentFns.insert(&F);
      }
    }
  }

  // clang-format off
  LLVM_DEBUG(
      dbgs() << "Leaf functions to be marked convergent:";
      if (ConvergentFns.empty()) {
        dbgs() << " (none)\n";
      } else {
        dbgs() << "\n";
        for (auto *F : ConvergentFns) {
          dbgs() << "  " << F->getName() << "\n";
        }
      }
  );
  // clang-format on

  if (ConvergentFns.empty()) {
    return PreservedAnalyses::all();
  }

  // Iterate over the convergent functions and recursively register all callers
  // of those functions as being convergent too.
  while (!Worklist.empty()) {
    Function *F = Worklist.pop_back_val();
    for (auto *U : F->users()) {
      if (auto *CB = dyn_cast<CallBase>(U)) {
        auto *Caller = CB->getFunction();
        if (ConvergentFns.insert(Caller).second) {
          Worklist.insert(Caller);
          ConvergentFns.insert(Caller);
          LLVM_DEBUG(dbgs() << "Function '" << Caller->getName()
                            << "' is transitively convergent\n");
        }
      } else {
        report_fatal_error("unhandled user type");
      }
    }
  }

  for (auto *F : ConvergentFns) {
    F->addFnAttr(Attribute::Convergent);
  }

  return PreservedAnalyses::none();
}
