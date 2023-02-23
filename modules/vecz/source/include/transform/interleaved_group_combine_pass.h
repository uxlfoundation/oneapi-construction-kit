// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Combine groups of interleaved memory operations.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_TRANSFORM_INTERLEAVED_GROUP_COMBINE_PASS_H_INCLUDED
#define VECZ_TRANSFORM_INTERLEAVED_GROUP_COMBINE_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

#include "analysis/uniform_value_analysis.h"
#include "vecz/vecz_target_info.h"

namespace llvm {
class ScalarEvolution;
}

namespace vecz {

class VectorizationUnit;

/// @brief Combine groups of interleaved memory operations.
class InterleavedGroupCombinePass
    : public llvm::PassInfoMixin<InterleavedGroupCombinePass> {
 public:
  /// @brief Create a new pass object.
  ///
  /// @param[in] kind Kind of interleaved operation to combine.
  InterleavedGroupCombinePass(InterleavedOperation kind)
      : Kind(kind), scalarEvolution(nullptr) {}

  /// @brief Unique identifier for the pass.
  static void *ID() { return (void *)&PassID; }

  /// @brief Combine groups of interleaved operations.
  ///
  /// @param[in] F Function to analyze.
  /// @param[in] AM FunctionAnalysisManager providing analyses.
  ///
  /// @return Preserved analyses.
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);

  /// @brief Pass name.
  static llvm::StringRef name() {
    return "Combine interleaved memory instructions";
  }

 private:
  /// @brief Information about an interleaved operation.
  struct InterleavedOpInfo;

  /// @brief Information about a group of interleaved operations.
  struct InterleavedGroupInfo;

  /// @brief Try to find a group of interleaved instructions that have the same
  /// stride and collectively access a consecutive chunk of memory.
  ///
  /// @param[in] Ops List of interleaved operations to analyze.
  /// @param[in] UVR Result of uniform value analysis.
  /// @param[out] Info information about a group of interleaved instructions.
  ///
  /// @return true if a group was found or false otherwise.
  bool findGroup(const std::vector<InterleavedOpInfo> &Ops,
                 UniformValueResult &UVR, InterleavedGroupInfo &Info);

  /// @brief Unique identifier for the pass.
  static char PassID;
  /// @brief Kind of interleaved operation to combine.
  InterleavedOperation Kind;

  /// @brief Scalar Evolution Analysis that allows us to subtract two pointers
  /// to find any constant offset between them.
  llvm::ScalarEvolution *scalarEvolution;
};

}  // namespace vecz

#endif  // VECZ_TRANSFORM_INTERLEAVED_GROUP_COMBINE_PASS_H_INCLUDED
