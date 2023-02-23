// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "analysis/control_flow_analysis.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/Analysis/CFG.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>

#include "analysis/uniform_value_analysis.h"
#include "debugging.h"

#define DEBUG_TYPE "vecz-cf"

using namespace llvm;
using namespace vecz;

////////////////////////////////////////////////////////////////////////////////

llvm::AnalysisKey CFGAnalysis::Key;

CFGResult CFGAnalysis::run(llvm::Function &F,
                           llvm::FunctionAnalysisManager &AM) {
  CFGResult Res;

  LLVM_DEBUG(dbgs() << "CONTROL FLOW ANALYSIS\n");

  UniformValueResult &UVR = AM.getResult<UniformValueAnalysis>(F);

  bool mayDiverge = false;
  for (BasicBlock &BB : F) {
    // Update diverge information for a block which has varying branch.
    auto *term = BB.getTerminator();
    if (isa<ReturnInst>(term) || isa<UnreachableInst>(term)) {
      // an "unreachable" terminator may be generated from an "optimization"
      // of undefined behaviour in the IR; where a "trap" call has been
      // introduced, the end of the Basic Block will never be reached.
      // This should still be regarded as an exit block for our purposes.
      if (Res.exitBB) {
        emitVeczRemarkMissed(&F, &F,
                             "CFG should not have more than one exit block.");
        Res.setFailed(true);
        return Res;
      }
      Res.exitBB = &BB;
      LLVM_DEBUG(dbgs() << BB.getName() << " returns\n");
    } else if (BranchInst *B = dyn_cast<BranchInst>(term)) {
      if (B->isConditional()) {
        auto *const cond = B->getCondition();
        if (cond && UVR.isVarying(cond)) {
          mayDiverge = true;
        }
      }
    } else if (isa<SwitchInst>(term)) {
      // Control Flow Conversion Pass is not able to handle switch instructions.
      emitVeczRemarkMissed(&F, &F, "Unexpected Switch instruction.");
      Res.setFailed(true);
      return Res;
    }
  }

  if (!Res.getExitBlock()) {
    emitVeczRemarkMissed(&F, &F, "Non-terminating CFG in");
    Res.setFailed(true);
    return Res;
  }

  LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
  using RPOTraversal = ReversePostOrderTraversal<const Function *>;
  RPOTraversal FuncRPOT(&F);
  if (containsIrreducibleCFG<const BasicBlock *, const RPOTraversal,
                             const LoopInfo>(FuncRPOT, LI)) {
    emitVeczRemarkMissed(&F, &F, "Irreducible loop detected in");
    Res.setFailed(true);
    return Res;
  }

  if (mayDiverge) {
    Res.setConversionNeeded(true);
  }

  return Res;
}
