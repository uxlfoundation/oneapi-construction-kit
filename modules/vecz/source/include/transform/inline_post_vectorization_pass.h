// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Replace calls to certain builtins with an inline implementation after
/// vectorization.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_TRANSFORM_INLINE_POST_VECTORIZATION_PASS_H_INCLUDED
#define VECZ_TRANSFORM_INLINE_POST_VECTORIZATION_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace vecz {

/// @brief This pass replaces calls to builtins that require special attention
/// after vectorization.
class InlinePostVectorizationPass
    : public llvm::PassInfoMixin<InlinePostVectorizationPass> {
 public:
  /// @brief Create a new pass object.
  InlinePostVectorizationPass() {}

  /// @brief The entry point to the pass.
  /// @param[in,out] F Function to optimize.
  /// @param[in,out] AM FunctionAnalysisManager providing analyses.
  /// @returns Whether or not the pass changed anything.
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
  /// @brief Retrieve the pass's name.
  /// @return pointer to text description.
  static llvm::StringRef name() { return "Inline Post Vectorization pass"; }
};
}  // namespace vecz

#endif  // VECZ_TRANSFORM_INLINE_POST_VECTORIZATION_PASS_H_INCLUDED
