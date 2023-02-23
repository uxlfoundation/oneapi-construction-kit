// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Class SoftwareDivisionPass interface.
///
/// Adds runtime checks to division instructions to prevent undefined behavior.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BASE_SOFTWARE_DIVISION_PASS_H_INCLUDED
#define BASE_SOFTWARE_DIVISION_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>

namespace llvm {
class Function;
}  // namespace llvm

namespace compiler {
/// @addtogroup cl_compiler
/// @{

class SoftwareDivisionPass final
    : public llvm::PassInfoMixin<SoftwareDivisionPass> {
 public:
  /// @brief The entry point to the pass.
  ///
  /// @param[in,out] F The function provided to the pass.
  /// @param[in,out] AM The manager providing function analyses to the pass.
  /// @return The set of preserved analyses.
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);

  // This pass is not an optimization
  static bool isRequired() { return true; }
};

/// @}
}  // namespace compiler

#endif  // BASE_SOFTWARE_DIVISION_PASS_H_INCLUDED
