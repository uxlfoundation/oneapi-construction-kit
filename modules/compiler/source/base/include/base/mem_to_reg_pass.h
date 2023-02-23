// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Simplify OpenCL builtins pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BASE_MEM_TO_REG_PASS_H_INCLUDED
#define BASE_MEM_TO_REG_PASS_H_INCLUDED

#include <llvm/IR/Dominators.h>
#include <llvm/IR/PassManager.h>

namespace compiler {
/// @addtogroup cl_compiler
/// @{

/// @brief Codeplay's mem to reg pass.
struct MemToRegPass final : public llvm::PassInfoMixin<MemToRegPass> {
  /// @brief The entry point to the MemToRegPass.
  ///
  /// @param[in,out] function The function to run the pass on.
  /// @param[in,out] am Analysis manager providing analyses.
  /// @return Whether or not the pass changed anything in the function.
  llvm::PreservedAnalyses run(llvm::Function &function,
                              llvm::FunctionAnalysisManager &am);
};

/// @}
}  // namespace compiler

#endif  // BASE_MEM_TO_REG_PASS_H_INCLUDED
