// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/remove_fences_pass.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;

PreservedAnalyses compiler::utils::RemoveFencesPass::run(
    Function &F, FunctionAnalysisManager &) {
  bool Changed = false;

  for (BasicBlock &BB : F) {
    for (auto InstI = BB.begin(); InstI != BB.end();) {
      auto &Inst = *InstI++;
      if (isa<FenceInst>(Inst)) {
        Inst.eraseFromParent();
        Changed = true;
      }
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
