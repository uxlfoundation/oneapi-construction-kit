// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Add floating-point control pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef HOST_ADD_FLOATING_POINT_CONTROL_PASS_H_INCLUDED
#define HOST_ADD_FLOATING_POINT_CONTROL_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace host {

/// @brief A pass that will replace any local work item functions.
///
/// Runs over all kernels with "kernel" metadata.
class AddFloatingPointControlPass final
    : public llvm::PassInfoMixin<AddFloatingPointControlPass> {
 public:
  AddFloatingPointControlPass(bool FTZ) : SetFTZ(FTZ) {}

  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);

 private:
  bool SetFTZ;
};
}  // namespace host

#endif  // HOST_ADD_FLOATING_POINT_CONTROL_PASS_H_INCLUDED
