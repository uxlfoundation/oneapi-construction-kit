// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief ROSCC control flow transformation.
///
/// Style guideline 004 exemption note: This inner class declaration is in its
/// own header to match `control_flow_boscc.h`.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_CONTROL_FLOW_ROSCC_H_INCLUDED
#define VECZ_CONTROL_FLOW_ROSCC_H_INCLUDED

#include "transform/control_flow_conversion_pass.h"

namespace llvm {
class Instruction;
class BasicBlock;
class Loop;
}  // namespace llvm

namespace vecz {

/// @brief class that encapsulates the ROSCC transformation, which stands for
///        "Return On Superword Condition Code" and optimizes non-uniform
///        branches to the function return block(s).
class ControlFlowConversionState::ROSCCGadget final {
 public:
  ROSCCGadget(ControlFlowConversionState &Pass)
      : UVR(Pass.UVR), DT(Pass.DT), PDT(Pass.PDT), LI(Pass.LI) {}

  /// @brief perform the ROSCC transformation
  bool run(llvm::Function &F);

 private:
  UniformValueResult *UVR = nullptr;
  llvm::DominatorTree *DT = nullptr;
  llvm::PostDominatorTree *PDT = nullptr;
  llvm::LoopInfo *LI = nullptr;
};
}  // namespace vecz

#endif  // VECZ_CONTROL_FLOW_ROSCC_H_INCLUDED
