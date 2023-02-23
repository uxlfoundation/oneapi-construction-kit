// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief SIMD width analysis.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_ANALYSIS_SIMD_WIDTH_ANALYSIS_H_INCLUDED
#define VECZ_ANALYSIS_SIMD_WIDTH_ANALYSIS_H_INCLUDED

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/PassManager.h>

#include "vectorization_unit.h"

namespace vecz {

class LivenessResult;

/// @brief Choose a good SIMD width for the given function.
class SimdWidthAnalysis : public llvm::AnalysisInfoMixin<SimdWidthAnalysis> {
  friend AnalysisInfoMixin<SimdWidthAnalysis>;

 public:
  /// @brief Create a new instance of the pass.
  SimdWidthAnalysis() = default;

  /// @brief Type of result produced by the analysis.
  struct Result {
    Result(unsigned value) : value(value) {}
    unsigned value;
  };

  /// @brief Run the SIMD width analysis pass on the given function.
  /// @param[in] F Function to analyze.
  /// @param[in] AM FunctionAnalysisManager providing analyses.
  /// @return Preferred SIMD vectorization factor for the function or zero.
  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);

  /// @brief Return the name of the pass.
  static llvm::StringRef name() { return "SIMD width analysis"; }

 private:
  unsigned avoidSpillImpl(llvm::Function &, llvm::FunctionAnalysisManager &,
                          unsigned MinWidth = 2);

  /// @brief Vector register width from TTI, if available.
  unsigned MaxVecRegBitWidth;

  /// @brief Unique pass identifier.
  static llvm::AnalysisKey Key;
};
}  // namespace vecz

#endif  // VECZ_ANALYSIS_SIMD_WIDTH_ANALYSIS_H_INCLUDED
