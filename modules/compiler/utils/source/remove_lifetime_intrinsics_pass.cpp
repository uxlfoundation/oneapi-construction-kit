// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
