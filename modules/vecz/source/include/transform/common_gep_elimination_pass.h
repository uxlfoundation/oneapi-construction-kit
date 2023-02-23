// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Remove duplicate GEP instructions.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_TRANSFORM_COMMON_GEP_ELIMINATION_PASS_H_INCLUDED
#define VECZ_TRANSFORM_COMMON_GEP_ELIMINATION_PASS_H_INCLUDED

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/PassManager.h>

namespace vecz {

class VectorizationUnit;

/// @brief This pass removes every duplicate GEP instruction before the
/// packetization pass.
class CommonGEPEliminationPass
    : public llvm::PassInfoMixin<CommonGEPEliminationPass> {
 public:
  static void *ID() { return (void *)&PassID; };

  /// @brief Remove duplicate GEP instructions.
  ///
  /// @param[in] F Function to optimize.
  /// @param[in] AM FunctionAnalysisManager providing analyses.
  ///
  /// @return Preserved passes.
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);

  /// @brief Pass name.
  static llvm::StringRef name() { return "Common GEP Elimination pass"; }

 private:
  /// @brief Identifier for the pass.
  static char PassID;
};
}  // namespace vecz

#endif  // VECZ_TRANSFORM_COMMON_GEP_ELIMINATION_PASS_H_INCLUDED
