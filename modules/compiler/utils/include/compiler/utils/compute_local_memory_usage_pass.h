// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// A pass to compute the local memory usage of entry-point functions
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_COMPUTE_LOCAL_MEMORY_USAGE_PASS_H_INCLUDED
#define COMPILER_UTILS_COMPUTE_LOCAL_MEMORY_USAGE_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

class ComputeLocalMemoryUsagePass final
    : public llvm::PassInfoMixin<ComputeLocalMemoryUsagePass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_COMPUTE_LOCAL_MEMORY_USAGE_PASS_H_INCLUDED
