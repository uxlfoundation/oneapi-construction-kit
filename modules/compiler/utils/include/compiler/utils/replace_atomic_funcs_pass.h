// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Replace async copies pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_REPLACE_ATOMIC_FUNCS_PASS_H_INCLUDED
#define COMPILER_UTILS_REPLACE_ATOMIC_FUNCS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass that will replace calls to the atomic builtins with calls to
/// the appropriate atomic llvm instructions.
class ReplaceAtomicFuncsPass final
    : public llvm::PassInfoMixin<ReplaceAtomicFuncsPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REPLACE_ATOMIC_FUNCS_PASS_H_INCLUDED
