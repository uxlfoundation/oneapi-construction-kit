// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#ifndef MULTI_LLVM_PASSMANAGER_ADAPTERS_H_INCLUDED
#define MULTI_LLVM_PASSMANAGER_ADAPTERS_H_INCLUDED

#include <llvm/Passes/PassBuilder.h>

namespace multi_llvm {

/** A simple mixin that allows creating legacy passes that call into a
   new-style pass. This should make it easier to move to the new PassManager
   infrastructure piecemeal
   e.g.

  ```
  using namespace llvm;
  struct MyModulePass
      : public LegacyPMAdapterMixin<ModulePass, ModulePassManager> {
    static char ID;
    MyModulePass()
        : LegacyPMAdapterMixin<ModulePass, ModulePassManager>(ID) {
          FAM.registerPass([]() { return SomeAnalysis(); });
      PM.addPass(NewStylePassToRun());
    }
    bool runOnModule(Module &M) override {
      return !this->PM.run(M, MAM).areAllPreserved();
    }
  };
  ```
*/
template <typename IRUnitPass, typename IRUnitManager>
class LegacyPMAdapterMixin : public IRUnitPass {
  // HEREBEDRAGONS: These need to be in this order to avoid weird destructor
  // ordering segfaults. The `crossRegisterProxies` function forces a dependency
  // between the various analyses, and if they're not destructed in the precise
  // order, double frees occur.
 public:
  IRUnitManager PM;
  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::ModuleAnalysisManager MAM;
  llvm::PassBuilder PB;

  LegacyPMAdapterMixin<IRUnitPass, IRUnitManager>(char &ID) : IRUnitPass(ID) {
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
  }
};
}  // namespace multi_llvm

#endif
