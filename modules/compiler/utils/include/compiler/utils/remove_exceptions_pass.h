// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Remove exceptions pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_REMOVE_EXCEPTIONS_PASS_H_INCLUDED
#define COMPILER_UTILS_REMOVE_EXCEPTIONS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass that adds the "nounwind" attribute to all functions.
///
/// There are various places where functions can be created that don't have a
/// "nounwind" attribute. This tells the backend that an exception can
/// potentially be thrown from here. However, this seems to badly confuse the
/// AArch64 backend for some reason. Since we don't actually support exceptions
/// in any case, we might as well add this attribute to everything.
class RemoveExceptionsPass final
    : public llvm::PassInfoMixin<RemoveExceptionsPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REMOVE_EXCEPTIONS_PASS_H_INCLUDED
