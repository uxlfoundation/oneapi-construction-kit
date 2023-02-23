// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Prepare barriers pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_PREPARE_BARRIERS_PASS_H_INCLUDED
#define COMPILER_UTILS_PREPARE_BARRIERS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief Pass for ensuring consistent barrier handling.
///
/// It inlines functions that contain barriers and gives each barrier call a
/// unique ID as metadata to ensure consistent handling of barriers in
/// different versions of the kernel (i.e. Scalar vs Vector). Run before Vecz
/// for mixed wrapper kernels made up of multiple kernels to work.
///
/// Runs over all kernels with "kernel entry point" metadata.
class PrepareBarriersPass final
    : public llvm::PassInfoMixin<PrepareBarriersPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_PREPARE_BARRIERS_PASS_H_INCLUDED
