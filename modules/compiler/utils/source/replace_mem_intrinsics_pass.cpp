// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/replace_mem_intrinsics_pass.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Transforms/Utils/LowerMemIntrinsics.h>
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
        expandMemMoveAsLoop(cast<MemMoveInst>(CI));
        break;
    }
    CI->eraseFromParent();
  }

  return !CallsToProcess.empty() ? PreservedAnalyses::none()
                                 : PreservedAnalyses::all();
}

}  // namespace utils
}  // namespace compiler
