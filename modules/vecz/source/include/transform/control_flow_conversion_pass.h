// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Control flow partial linearization transform.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_TRANSFORM_CONTROL_FLOW_CONVERSION_PASS_H_INCLUDED
#define VECZ_TRANSFORM_CONTROL_FLOW_CONVERSION_PASS_H_INCLUDED

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/PassManager.h>

#include <memory>

namespace llvm {
class Function;
class Value;
class DominatorTree;
class PostDominatorTree;
class PreservedAnalyses;
class LoopInfo;
}  // namespace llvm

namespace vecz {
struct BasicBlockTag;
struct LoopTag;
struct UniformValueResult;
class DivergenceResult;
class VectorizationUnit;
class VectorizationContext;
class Reachability;

/// \addtogroup cfg-conversion Control Flow Conversion Stage
/// @{
/// \ingroup vecz

/// @brief Pass that convert performs control-flow to data-flow conversion for
/// a function.
class ControlFlowConversionPass
    : public llvm::PassInfoMixin<ControlFlowConversionPass> {
 public:
  /// @brief Unique identifier for the pass.
  static void *ID() { return (void *)&PassID; }

  /// @brief Perform control-flow to data-flow conversion on the function's CFG.
  ///
  /// @param[in] F Function to convert.
  /// @param[in] AM FunctionAnalysisManager providing analyses.
  ///
  /// @return Preserved analyses.
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);

  /// @brief Pass name.
  static llvm::StringRef name() {
    return "Control flow to data flow conversion";
  }

 private:
  /// @brief Unique identifier for the pass.
  static char PassID;
};

class ControlFlowConversionState {
 public:
  /// @brief The actual implementation of this pass
  class Impl;

 protected:
  ControlFlowConversionState(llvm::Function &,
                             llvm::FunctionAnalysisManager &AM);

  /// @brief BOSCC (Branch On Superword Codition Code) data structure that
  ///        encloses regions of the CFG that contain blocks that need to be
  ///        duplicated.
  class BOSCCGadget;

  /// @brief ROSCC (Return On Superword Codition Code) utility class to
  ///        optimize conditional function return branches.
  class ROSCCGadget;

  llvm::Function &F;
  llvm::FunctionAnalysisManager &AM;
  VectorizationUnit &VU;
  VectorizationContext &Ctx;
  llvm::DominatorTree *DT = nullptr;
  llvm::PostDominatorTree *PDT = nullptr;
  llvm::LoopInfo *LI = nullptr;
  DivergenceResult *DR = nullptr;
  UniformValueResult *UVR = nullptr;
  std::unique_ptr<BOSCCGadget> BOSCC;
  std::unique_ptr<Reachability> RC;

 private:
  struct MaskInfo {
    /// @brief Mask that describes which lanes have exited the block.
    llvm::SmallDenseMap<llvm::BasicBlock *, llvm::Value *, 4> exitMasks;
    /// @brief Mask that describes which lanes are active at the start of the
    /// basic block.
    llvm::Value *entryMask = nullptr;
  };
  llvm::DenseMap<llvm::BasicBlock *, MaskInfo> MaskInfos;

  /// @brief get the Mask Info struct for a Basic Block.
  /// Note that the returned reference may be invalidated by subsequent calls.
  ///
  /// @param[in] BB the BasicBlock
  /// @returns a reference to the MaskInfo
  const MaskInfo &getMaskInfo(llvm::BasicBlock *BB) const {
    auto const found = MaskInfos.find(BB);
    assert(found != MaskInfos.end() &&
           "Mask Info not constructed for Basic Block!");
    return found->second;
  }

  /// @brief replaces reachable uses of a value
  ///
  /// @param[in] RC the reachability computation to use
  /// @param[in] from the value to replace
  /// @param[in] to the value to substitute
  /// @param[in] src the basic block from which the value must be reachable
  ///
  /// @returns true
  static bool replaceReachableUses(Reachability &RC, llvm::Instruction *from,
                                   llvm::Value *to, llvm::BasicBlock *src);

  /// @brief Generate a block ordering.
  ///
  /// This is based on a dominance-compact block indexing (DCBI) where we
  /// topologically order blocks that belong to the same dominator tree.
  ///
  /// @returns true if no errors occurred.
  bool computeBlockOrdering();
};

/// @}
}  // namespace vecz

#endif  // VECZ_TRANSFORM_CONTROL_FLOW_CONVERSION_PASS_H_INCLUDED
