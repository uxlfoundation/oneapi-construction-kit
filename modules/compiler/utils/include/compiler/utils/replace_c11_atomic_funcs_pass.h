// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Replace C11 atomic functions pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_REPLACE_C11_ATOMIC_FUNCS_PASS_H_INCLUDED
#define COMPILER_UTILS_REPLACE_C11_ATOMIC_FUNCS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass that changes C11 atomic functions to atomic instructions.
///
/// This pass replaces the subset of C11 atomic builtins we support with LLVM
/// instructions. Since we only support the minum requirements this pass does
/// not implement all the C11 atomic builtins e.g. atomic_(load|store) are not
/// implemented. Atomics are detected based on their mangled names, changes to
/// the name mangling ABI will cause this pass to break if it is not updated.
class ReplaceC11AtomicFuncsPass final
    : public llvm::PassInfoMixin<ReplaceC11AtomicFuncsPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REPLACE_C11_ATOMIC_FUNCS_PASS_H_INCLUDED
