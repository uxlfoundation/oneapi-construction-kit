// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Rename Builtins pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_RENAME_BUILTINS_PASS_H_INCLUDED
#define COMPILER_UTILS_RENAME_BUILTINS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass that will rename all function names (and their uses) beginning
/// with the __mux prefix back to the original __core prefix.

class RenameBuiltinsPass final
    : public llvm::PassInfoMixin<RenameBuiltinsPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_RENAME_BUILTINS_PASS_H_INCLUDED
