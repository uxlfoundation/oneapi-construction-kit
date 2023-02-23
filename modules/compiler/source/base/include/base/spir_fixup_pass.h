// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Fix SPIR IR pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BASE_SPIR_FIXUP_PASS_H_INCLUDED
#define BASE_SPIR_FIXUP_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace spir {
/// @addtogroup cl_compiler
/// @{

/// @brief Pass to fix SPIR IR for the rest of compiler passes.
struct SpirFixupPass final : public llvm::PassInfoMixin<SpirFixupPass> {
  /// @brief Constructor.
  SpirFixupPass() = default;

  /// @brief The entry point to the SpirFixupPass.
  /// @param[in,out] module The module to run the pass on.
  /// @return Whether or not the pass changed anything in the module.
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};

/// @}
}  // namespace spir
}  // namespace compiler

#endif  // BASE_SPIR_FIXUP_PASS_H_INCLUDED
