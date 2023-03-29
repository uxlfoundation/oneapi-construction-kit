// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Adds 3D work-group loops around a kernel
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef RISCV_REFSI_WG_LOOP_PASS_H_INCLUDED
#define RISCV_REFSI_WG_LOOP_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace refsi_g1_wi {

class RefSiWGLoopPass final : public llvm::PassInfoMixin<RefSiWGLoopPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};

};  // namespace refsi_g1_wi

#endif  // RISCV_REFSI_WG_LOOP_PASS_H_INCLUDED
