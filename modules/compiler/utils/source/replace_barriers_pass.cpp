// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/builtin_info.h>
#include <compiler/utils/replace_barriers_pass.h>
#include <llvm/IR/Instructions.h>

#include <cassert>

using namespace llvm;

PreservedAnalyses compiler::utils::ReplaceBarriersPass::run(
    Module &M, ModuleAnalysisManager &AM) {
  auto &BI = AM.getResult<BuiltinInfoAnalysis>(M);

  SmallVector<CallInst *, 8> Calls;
  for (auto &F : M.functions()) {
    auto B = BI.analyzeBuiltin(F);
    if (B.properties & eBuiltinPropertyMapToMuxSyncBuiltin) {
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
    if (auto *const NewCI = BI.mapSyncBuiltinToMuxSyncBuiltin(*CI)) {
      CI->replaceAllUsesWith(NewCI);
      CI->eraseFromParent();
    }
  }

  return PreservedAnalyses::none();
}
