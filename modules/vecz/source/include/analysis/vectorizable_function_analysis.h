// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Vectorizable Function analysis.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_ANALYSIS_VECTORIZABLE_FUNCTION_ANALYSIS_H_INCLUDED
#define VECZ_ANALYSIS_VECTORIZABLE_FUNCTION_ANALYSIS_H_INCLUDED

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/PassManager.h>

namespace vecz {

/// @brief Determines whether vectorization of a function is possible.
class VectorizableFunctionAnalysis
    : public llvm::AnalysisInfoMixin<VectorizableFunctionAnalysis> {
  friend AnalysisInfoMixin<VectorizableFunctionAnalysis>;

 public:
  /// @brief Create a new instance of the pass.
  VectorizableFunctionAnalysis() = default;

  /// @brief Type of result produced by the analysis.
  struct Result {
    /// @brief Whether the function can be vectorized.
    bool canVectorize = false;

    /// @brief If the function can not be vectorized, the value (if any) that
    /// is the cause of the problem.
    llvm::Value const *failedAt = nullptr;

   public:
    /// @brief Handle invalidation events from the new pass manager.
    ///
    /// @return false, as this analysis can never be invalidated.
    bool invalidate(llvm::Function &, const llvm::PreservedAnalyses &,
                    llvm::FunctionAnalysisManager::Invalidator &) {
      return false;
    }
  };

  /// @brief Determine whether vectorization of a function is possible.
  /// @param[in] F Function to analyze.
  /// @return VectorizationUnit corresponding to this function
  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &);

  /// @brief Return the name of the pass.
  static llvm::StringRef name() { return "Vectorizable Function analysis"; }

 private:
  /// @brief Unique pass identifier.
  static llvm::AnalysisKey Key;
};

}  // namespace vecz

#endif  // VECZ_ANALYSIS_VECTORIZABLE_FUNCTION_ANALYSIS_H_INCLUDED
