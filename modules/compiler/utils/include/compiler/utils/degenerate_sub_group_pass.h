// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Replaces calls to sub-group builtins with their analogous work-group
/// builtin.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_DEGENERATE_SUB_GROUP_PASS_H_INCLUDED
#define COMPILER_UTILS_DEGENERATE_SUB_GROUP_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief Provides the "degenerate" sub-group implementation where a sub-group
/// is an entire work-group and so any sub-group builtin call is equivalent to
/// the corresponding work-group builtin call.
class DegenerateSubGroupPass final
    : public llvm::PassInfoMixin<DegenerateSubGroupPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_DEGENERATE_SUB_GROUP_PASS_H_INCLUDED
