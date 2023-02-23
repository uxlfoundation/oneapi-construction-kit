// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Replace work-group collectives pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_REPLACE_WGC_PASS_H_INCLUDED
#define COMPILER_UTILS_REPLACE_WGC_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief Provides a default implementation of the work-group collective
/// builtins using local memory as an accumulator. Targets with no hardware
/// support for work-group collectives may use this pass to provide a software
/// emulation.
///
/// This pass introduces barrier calls into the work-group collective
/// definitions so must be run before the PrepareBarriersPass and
/// HandleBarriersPass on any target making use of these passes. This pass also
/// introduces global variables into the module in the __local address space and
/// therefore must be run before the ReplaceLocalModuleScopeVariablesPass on any
/// target making use of that pass.
class ReplaceWGCPass final : public llvm::PassInfoMixin<ReplaceWGCPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REPLACE_WGC_PASS_H_INCLUDED
