#ifndef REFSI_M1_WRAPPER_PASS_H_INCLUDED
#define REFSI_M1_WRAPPER_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace refsi_m1 {
class RefSiM1WrapperPass final
    : public llvm::PassInfoMixin<RefSiM1WrapperPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace refsi_m1

#endif
