// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Scalar/LoopRotation.h"
#include "transform/passes.h"

using namespace llvm;

llvm::PreservedAnalyses vecz::VeczLoopRotatePass::run(
    llvm::Loop &L, llvm::LoopAnalysisManager &LAM,
    llvm::LoopStandardAnalysisResults &AR, llvm::LPMUpdater &LU) {
  // Only process loops whose latch cannot exit the loop and its predecessors
  // cannot either.
  if (L.isLoopExiting(L.getLoopLatch())) {
    return PreservedAnalyses::all();
  }

  for (BasicBlock *pred : predecessors(L.getLoopLatch())) {
    if (L.contains(pred) && L.isLoopExiting(pred)) {
      return PreservedAnalyses::all();
    }
  }

  return LoopRotatePass().run(L, LAM, AR, LU);
}
