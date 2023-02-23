// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/address_spaces.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/compute_local_memory_usage_pass.h>
#include <compiler/utils/metadata.h>
#include <llvm/ADT/PriorityWorklist.h>
#include <llvm/Analysis/LazyCallGraph.h>

#define DEBUG_TYPE "compute-local-memory-usage"

using namespace llvm;

namespace compiler {
namespace utils {

PreservedAnalyses ComputeLocalMemoryUsagePass::run(Module &M,
                                                   ModuleAnalysisManager &AM) {
  const auto &DL = M.getDataLayout();
  LazyCallGraph &G = AM.getResult<LazyCallGraphAnalysis>(M);

  for (auto &F : M) {
    // Only compute local memory usages for entry points
    if (!isKernelEntryPt(F)) {
      continue;
    }

    // Get the lazy CG node.
    auto &N = G.get(F);

    SmallPtrSet<Function *, 4> VisitedFns;
    SmallPtrSet<LazyCallGraph::Node *, 4> Visited = {&N};

    // Construct a work list to collect the list of defined functions in the
    // graph. Note that this won't walk function declarations, but that's okay
    // as we wouldn't be able detect any local memory usage in those functions
    // anyway.
    SmallPriorityWorklist<LazyCallGraph::Node *, 4> Worklist;
    Worklist.insert(&N);

    LLVM_DEBUG(dbgs() << "Edges in function '" << N.getFunction().getName()
                      << "':\n");

    while (!Worklist.empty()) {
      LazyCallGraph::Node *N = Worklist.pop_back_val();
      VisitedFns.insert(&N->getFunction());
      for (LazyCallGraph::Edge &E : N->populate()) {
        LLVM_DEBUG(dbgs() << "    " << (E.isCall() ? "call" : "ref ") << " -> "
                          << E.getFunction().getName() << "\n");
        if (Visited.insert(&E.getNode()).second) {
          Worklist.insert(&E.getNode());
        }
      }
    }

    uint64_t LocalMemorySizeTotal = 0;
    LLVM_DEBUG(dbgs() << "Local-memory global usage:\n");
    for (const auto &GV : M.globals()) {
      if (GV.getAddressSpace() != AddressSpace::Local) {
        continue;
      }

      // Only count globals used in some form by any of the functions in the
      // call graph.
      if (none_of(GV.users(), [&VisitedFns](const User *U) {
            return isa<Instruction>(U) &&
                   VisitedFns.count(cast<Instruction>(U)->getFunction());
          })) {
        LLVM_DEBUG(dbgs() << "  GV '" << GV.getName() << "' is unused\n");
        continue;
      }

      if (auto *const ValTy = GV.getValueType()) {
        LLVM_DEBUG(dbgs() << "  GV '" << GV.getName() << "' ("
                          << DL.getTypeAllocSize(ValTy) << " bytes)\n");
        LocalMemorySizeTotal += DL.getTypeAllocSize(ValTy);
      }
    }

    LLVM_DEBUG(dbgs() << "Usage total: " << LocalMemorySizeTotal
                      << " bytes\n\n");
    setLocalMemoryUsage(F, LocalMemorySizeTotal);
  }

  return PreservedAnalyses::all();
}

}  // namespace utils
}  // namespace compiler
