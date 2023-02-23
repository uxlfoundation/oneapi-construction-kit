// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Simplify OpenCL builtins pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BASE_BUILTIN_SIMPLIFICATION_PASS_H_INCLUDED
#define BASE_BUILTIN_SIMPLIFICATION_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
/// @addtogroup cl_compiler
/// @{

/// @brief Post parsing pass to make a module work for fast math.
struct BuiltinSimplificationPass final
    : public llvm::PassInfoMixin<BuiltinSimplificationPass> {
  /// @brief The entry point to the BuiltinSimplificationPass.
  /// @param[in,out] module The module to run the pass on.
  /// @param[in,out] am Analysis manager providing analyses.
  /// @return Whether or not the pass changed anything in the module.
  llvm::PreservedAnalyses run(llvm::Module &module,
                              llvm::ModuleAnalysisManager &am);
};

/// @}
}  // namespace compiler

#endif  // BASE_BUILTIN_SIMPLIFICATION_PASS_H_INCLUDED
