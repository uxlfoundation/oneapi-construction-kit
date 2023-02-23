// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file liveness_analysis.h
///
/// @brief Live Variable Set Analysis
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_ANALYSIS_LIVENESS_ANALYSIS_H
#define VECZ_ANALYSIS_LIVENESS_ANALYSIS_H

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/PassManager.h>

namespace llvm {
class Loop;
class LoopInfo;
class Function;
class BasicBlock;
class Value;
}  // namespace llvm

namespace vecz {
class VectorizationUnit;

struct BlockLivenessInfo {
  using LiveSet = llvm::SmallVector<llvm::Value *, 16>;

  LiveSet LiveIn;
  LiveSet LiveOut;
  size_t MaxRegistersInBlock = 0;
};

class LivenessResult {
 public:
  LivenessResult(llvm::Function &F) : F(F) {}

  LivenessResult() = delete;
  LivenessResult(const LivenessResult &) = delete;
  LivenessResult(LivenessResult &&) = default;
  ~LivenessResult() = default;

  void recalculate();

  size_t getMaxLiveVirtualRegisters() const;
  const BlockLivenessInfo &getBlockInfo(const llvm::BasicBlock *) const;

 private:
  class Impl;

  llvm::Function &F;

  size_t maxNumberOfLiveValues;

  llvm::DenseMap<const llvm::BasicBlock *, BlockLivenessInfo> BlockInfos;
};

/// Analysis pass to perform liveness analysis and estimate register pressure by
/// counting the number of live virtual registers in a function.
///
/// Values in a basic block's live set are guaranteed to be in program order.
class LivenessAnalysis : public llvm::AnalysisInfoMixin<LivenessAnalysis> {
  friend llvm::AnalysisInfoMixin<LivenessAnalysis>;

 public:
  using Result = LivenessResult;

  LivenessAnalysis() = default;

  /// @brief Return the name of the pass.
  static llvm::StringRef name() { return "Liveness analysis"; }

  /// Estimate the number of registers needed by F by counting the number of
  /// live values.
  ///
  /// Assumes a reducible CFG. In OpenCL 1.2 whether or not irreducible control
  /// flow is illegal is implementation defined.
  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &);

  /// @brief Unique pass identifier.
  static llvm::AnalysisKey Key;
};

}  // namespace vecz

#endif  // VECZ_ANALYSIS_LIVENESS_ANALYSIS_H
