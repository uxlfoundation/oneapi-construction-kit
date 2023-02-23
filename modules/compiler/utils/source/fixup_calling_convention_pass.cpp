// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/attributes.h>
#include <compiler/utils/fixup_calling_convention_pass.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;

PreservedAnalyses compiler::utils::FixupCallingConventionPass::run(
    Module &M, ModuleAnalysisManager &) {
  bool Changed = false;

  for (auto &F : M.functions()) {
    // set the calling convention to system default
    // don't update calling convention for intrinsics
    if (F.isIntrinsic()) {
      continue;
    }

    CallingConv::ID ActualCC;
    if (CC == CallingConv::SPIR_KERNEL || CC == CallingConv::SPIR_FUNC) {
      if (isKernel(F)) {
        ActualCC = CallingConv::SPIR_KERNEL;
      } else {
        ActualCC = CallingConv::SPIR_FUNC;
      }
    } else {
      ActualCC = CC;
    }

    if (F.getCallingConv() != ActualCC) {
      Changed = true;
      // set the calling convention of the function
      F.setCallingConv(ActualCC);
    }

    // go through all users of the function
    for (auto &Use : F.uses()) {
      auto CI = dyn_cast<CallInst>(Use.getUser());

      // if we had a call instruction
      if (CI && CI->getCallingConv() != ActualCC) {
        Changed = true;
        // set the calling convention of the call too
        CI->setCallingConv(ActualCC);
      }
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
