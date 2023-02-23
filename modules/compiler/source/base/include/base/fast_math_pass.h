// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Compiler related internal ocl header file.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BASE_FAST_MATH_PASS_H_INCLUDED
#define BASE_FAST_MATH_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
/// @addtogroup cl_compiler
/// @{

/// @brief Post parsing pass to make a module work for fast math.
struct FastMathPass final : public llvm::PassInfoMixin<FastMathPass> {
  FastMathPass() = default;

  /// @brief The entry point to the FastMathPass.
  /// @param[in,out] module The module to run the pass on.
  /// @return Whether or not the pass changed anything in the module.
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
};

/// @}
}  // namespace compiler

#endif  // BASE_FAST_MATH_PASS_H_INCLUDED
