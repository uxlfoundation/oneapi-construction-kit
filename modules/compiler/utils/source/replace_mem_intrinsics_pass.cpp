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

#include <compiler/utils/replace_mem_intrinsics_pass.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Transforms/Utils/LowerMemIntrinsics.h>
#include <multi_llvm/llvm_version.h>

using namespace llvm;

namespace compiler {
namespace utils {

PreservedAnalyses ReplaceMemIntrinsicsPass::run(Function &F,
                                                FunctionAnalysisManager &FAM) {
  TargetTransformInfo &TTI = FAM.getResult<TargetIRAnalysis>(F);
  SmallVector<CallInst *, 4> CallsToProcess;

  for (auto &B : F) {
    for (Instruction &I : B) {
      if (auto *CI = dyn_cast<CallInst>(&I)) {
        switch (CI->getIntrinsicID()) {
          case Intrinsic::memcpy:
          case Intrinsic::memset:
            CallsToProcess.push_back(CI);
            break;
          case Intrinsic::memmove:
            // The expandMemMoveAsLoop fails if the address spaces differ so we
            // will only do it if they are the same - see CA-4682
            if (CI->getArgOperand(0)->getType()->getPointerAddressSpace() ==
                CI->getArgOperand(1)->getType()->getPointerAddressSpace()) {
              CallsToProcess.push_back(CI);
            }
            break;
        }
      }
    }
  }

  for (auto *CI : CallsToProcess) {
    switch (CI->getIntrinsicID()) {
      case Intrinsic::memcpy:
        expandMemCpyAsLoop(cast<MemCpyInst>(CI), TTI);
        break;
      case Intrinsic::memset:
        expandMemSetAsLoop(cast<MemSetInst>(CI));
        break;
      case Intrinsic::memmove:
#if LLVM_VERSION_GREATER_EQUAL(17, 0)
        expandMemMoveAsLoop(cast<MemMoveInst>(CI), TTI);
#else
        expandMemMoveAsLoop(cast<MemMoveInst>(CI));
#endif
        break;
    }
    CI->eraseFromParent();
  }

  return !CallsToProcess.empty() ? PreservedAnalyses::none()
                                 : PreservedAnalyses::all();
}

}  // namespace utils
}  // namespace compiler
