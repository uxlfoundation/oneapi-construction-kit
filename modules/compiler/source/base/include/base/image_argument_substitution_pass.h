// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Class ImageArgumentSubstitutionPass interface.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BASE_IMAGE_ARGUMENT_SUBSTITUTION_PASS_H_INCLUDED
#define BASE_IMAGE_ARGUMENT_SUBSTITUTION_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
/// @addtogroup cl_compiler
/// @{

/// @brief Replace OpenCL image calls with those coming from image library.
struct ImageArgumentSubstitutionPass final
    : public llvm::PassInfoMixin<ImageArgumentSubstitutionPass> {
  /// @brief Replace OpenCL opaque image types with those from image library.
  ///
  /// Overrides llvm::ModulePass::runOnModule().
  ///
  /// @param[in,out] module The module to run the pass on.
  /// @param[in,out] am The analysis manager providing module analyses.
  ///
  /// @return Return the set of preserved analyses.
  llvm::PreservedAnalyses run(llvm::Module &module,
                              llvm::ModuleAnalysisManager &am);
};

/// @}
}  // namespace compiler

#endif  // BASE_IMAGE_ARGUMENT_SUBSTITUTION_PASS_H_INCLUDED
