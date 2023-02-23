// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Remove fences pass
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_REMOVE_FENCES_PASS_H_INCLUDED
#define COMPILER_UTILS_REMOVE_FENCES_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass that removes all memory fence instructions.
///
/// ENORMOUS WARNING:
/// Removing memory fences can result in invalid code or incorrect behaviour in
/// general. This pass is a workaround for backends that do not yet support
/// memory fences.
class RemoveFencesPass final : public llvm::PassInfoMixin<RemoveFencesPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REMOVE_FENCES_PASS_H_INCLUDED
