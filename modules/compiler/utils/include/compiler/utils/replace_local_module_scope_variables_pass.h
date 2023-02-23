// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Replace local module-scope variables pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_REPLACE_LOCAL_MODULE_SCOPE_VARIABLES_PASS_H_INCLUDED
#define COMPILER_UTILS_REPLACE_LOCAL_MODULE_SCOPE_VARIABLES_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief __local address space automatic variables are represented in the
/// LLVM module as global variables with address space 3. This pass identifies
/// these variables and places them into a struct allocated (via alloca) in a
/// newly created wrapper function. A pointer to the struct is then passed
/// via a parameter to the original kernel.
///
/// Runs over all kernels with "kernel" metadata.
class ReplaceLocalModuleScopeVariablesPass final
    : public llvm::PassInfoMixin<ReplaceLocalModuleScopeVariablesPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REPLACE_LOCAL_MODULE_SCOPE_VARIABLES_PASS_H_INCLUDED
