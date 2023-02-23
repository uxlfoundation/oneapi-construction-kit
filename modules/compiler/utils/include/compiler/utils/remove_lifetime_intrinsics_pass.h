// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Remove llvm lifetime intrinsics pass
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_REMOVE_LIFETIME_INTRINSICS_PASS_H_INCLUDED
#define COMPILER_UTILS_REMOVE_LIFETIME_INTRINSICS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass for removing lifetime intrinsics from all instructions in a
/// module
///
/// The function pass `RemoveLifetimeIntrinsics` removes the
/// `llvm.lifetime.start` and `llvm.lifetime.end` intrinsics from a module by
/// iterating over all the instructions and erasing any lifetime intrinsics
/// found, as well as the bitcasts they use for the pointer argument. Removing
/// this information is useful for debugging since the backend is less likely to
/// optimize away variables in the stack no longer used, as a result this pass
/// should only be run on debug builds of the module.
class RemoveLifetimeIntrinsicsPass final
    : public llvm::PassInfoMixin<RemoveLifetimeIntrinsicsPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REMOVE_LIFETIME_INTRINSICS_PASS_H_INCLUDED
