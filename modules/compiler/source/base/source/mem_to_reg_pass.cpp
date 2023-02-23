// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
