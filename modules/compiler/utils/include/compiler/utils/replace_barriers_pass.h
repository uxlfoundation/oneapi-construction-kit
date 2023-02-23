// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Replace barriers pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_REPLACE_BARRIERS_PASS_H_INCLUDED
#define COMPILER_UTILS_REPLACE_BARRIERS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass that will replace calls to the barrier builtin with calls to
/// the core barrier functions

class ReplaceBarriersPass final
    : public llvm::PassInfoMixin<ReplaceBarriersPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REPLACE_BARRIERS_PASS_H_INCLUDED
