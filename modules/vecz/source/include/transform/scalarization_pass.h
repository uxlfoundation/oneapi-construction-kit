// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Function scalarizer.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_TRANSFORM_SCALARIZATION_PASS_H_INCLUDED
#define VECZ_TRANSFORM_SCALARIZATION_PASS_H_INCLUDED

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/PassManager.h>

namespace llvm {
class Function;
}  // namespace llvm

namespace vecz {

class VectorizationUnit;

/// \addtogroup scalarization Scalarization Stage
/// @{
/// \ingroup vecz

/// @brief Scalarization pass where vector instructions that need it are
/// scalarized, starting from leaves.
class ScalarizationPass : public llvm::PassInfoMixin<ScalarizationPass> {
 public:
  /// @brief Create a new scalarizaation pass.
  ScalarizationPass();

  /// @brief Unique identifier for the pass.
  static void *ID() { return (void *)&PassID; }

  /// @brief Scalarize the given function.
  ///
  /// @param[in] F Function to scalarize.
  /// @param[in] AM FunctionAnalysisManager providing analyses.
  ///
  /// @return Preserved analyses.
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);

  /// @brief Name of the pass.
  static llvm::StringRef name() { return "Function scalarization"; }

 private:
  static char PassID;
};

/// @}
}  // namespace vecz

#endif  // VECZ_TRANSFORM_SCALARIZATION_PASS_H_INCLUDED
