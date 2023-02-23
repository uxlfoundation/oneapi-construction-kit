// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/builtin_info.h>
#include <compiler/utils/define_mux_builtins_pass.h>

#define DEBUG_TYPE "define-mux-builtins"

using namespace llvm;

PreservedAnalyses compiler::utils::DefineMuxBuiltinsPass::run(
    Module &M, ModuleAnalysisManager &AM) {
  bool Changed = false;
  auto &BI = AM.getResult<BuiltinInfoAnalysis>(M);

  auto functionNeedsDefining = [&BI](Function &F) {
    return F.isDeclaration() && !F.isIntrinsic() &&
           BI.isMuxBuiltinID(BI.analyzeBuiltin(F).ID);
  };

  // Define all mux builtins
  for (auto &F : M.functions()) {
    if (!functionNeedsDefining(F)) {
      continue;
    }
    LLVM_DEBUG(dbgs() << "  Defining mux builtin: " << F.getName() << "\n";);

    // Define the builtin. If it declares any new dependent builtins, those
    // will be appended to the module's function list and so will be
    // encountered by later iterations.
    if (BI.defineMuxBuiltin(BI.analyzeBuiltin(F).ID, M)) {
      Changed = true;
    }
  }

  // While declaring any builtins should go to the end of the module's list of
  // functions, it's not technically impossible for something else to happen.
  // As such, assert that we are leaving the module in the state we are
  // contractually obliged to: with all functions that need defining having
  // been defined.
  assert(all_of(M.functions(),
                [&](Function &F) {
                  return F.isDeclaration() || !functionNeedsDefining(F);
                }) &&
         "Did not define a function that requires it");

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
