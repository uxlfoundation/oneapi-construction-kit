// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Class SetConvergentAttrPass interface.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BASE_SET_CONVERGENT_ATTR_PASS_H_INCLUDED
#define BASE_SET_CONVERGENT_ATTR_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
/// @addtogroup cl_compiler
/// @{

/// @brief Set the convergent attribute on convergent functions.
struct SetConvergentAttrPass final
    : public llvm::PassInfoMixin<SetConvergentAttrPass> {
  SetConvergentAttrPass() = default;

  /// @brief The entry point to the pass.
  ///
  /// @param[in,out] M The module provided to the pass.
  ///
  /// @return Return whether or not the pass changed anything.
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);
};

/// @}
}  // namespace compiler

#endif  // BASE_SET_CONVERGENT_ATTR_PASS_H_INCLUDED
