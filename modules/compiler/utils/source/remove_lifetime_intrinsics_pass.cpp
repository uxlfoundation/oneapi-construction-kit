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

#include <compiler/utils/remove_lifetime_intrinsics_pass.h>
#include <llvm/IR/IntrinsicInst.h>

using namespace llvm;

/// @brief Removes all llvm lifetime intrinsics from the function.
///
/// Erasing lifetime intrinsics is useful for debugging since the backend is
/// less likely to optimize away variables on the stack that are no longer used,
/// as a result this pass should only be run for debug compilation builds.
PreservedAnalyses compiler::utils::RemoveLifetimeIntrinsicsPass::run(
    Function &F, FunctionAnalysisManager &) {
  SmallVector<Instruction *, 8> toDelete;

  // Iterate over all instructions in function looking for
  // `llvm.lifetime.start`/`llvm.lifetime.end` intrinsics.
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (auto *intrinsic = dyn_cast<IntrinsicInst>(&I)) {
        if (intrinsic->getIntrinsicID() == Intrinsic::lifetime_end ||
            intrinsic->getIntrinsicID() == Intrinsic::lifetime_start) {
          // Mark intrinsic for deletion
          toDelete.push_back(intrinsic);

          // Second argument to intrinsic is a pointer bitcasted to `i8*`,
          // this can be removed since the lifetime intrinsic is the only use.
          auto bitcast = dyn_cast<BitCastInst>(intrinsic->getArgOperand(1));
          if (bitcast && bitcast->hasOneUse()) {
            toDelete.push_back(bitcast);
          }
        }
      }
    }
  }

  // Delete all the lifetime intrinsics and associated bitcasts we've found
  for (auto I : toDelete) {
    I->eraseFromParent();
  }

  return toDelete.empty() ? PreservedAnalyses::all()
                          : PreservedAnalyses::none();
}
