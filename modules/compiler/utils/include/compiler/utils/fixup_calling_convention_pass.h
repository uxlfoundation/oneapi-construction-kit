// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Fix up calling conventions pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_FIXUP_CALLING_CONVENTION_PASS_H_INCLUDED
#define COMPILER_UTILS_FIXUP_CALLING_CONVENTION_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass for making sure the calling convention of the target
/// executable matches the system default.
///
/// To make sure that the calling convention of the target executable matches
/// the system default calling convention `FixupCallingConventionPass` can be
/// run as a module pass. `FixupCallingConventionPass` iterates over all the
/// functions in the executable module, and if that function is not an
/// intrinsic, updates the calling convention of the function and all its call
/// instruction callees.
///
/// Runs over all kernels with "kernel" metadata.
///
/// @param cc An LLVM Calling Convention ID to be applied to all non-intrinsic
/// functions in the module.
class FixupCallingConventionPass final
    : public llvm::PassInfoMixin<FixupCallingConventionPass> {
 public:
  FixupCallingConventionPass(llvm::CallingConv::ID CC) : CC(CC) {}

  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);

 private:
  llvm::CallingConv::ID CC;
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_FIXUP_CALLING_CONVENTION_PASS_H_INCLUDED
