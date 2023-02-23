// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Workaround LLVM x86 bug by removing 'byval' parameter attributes.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef HOST_REMOVE_BYVAL_ATTRIBUTES_PASS_H_INCLUDED
#define HOST_REMOVE_BYVAL_ATTRIBUTES_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace host {

/// @brief This pass removes all 'byval' parameter attributes from functions
/// when compiling for x86-64 targets.
///
/// It works around an LLVM x86-64 codegen bug
/// https://github.com/llvm/llvm-project/issues/34300 where byval parameters
/// are incorrectly lowered across call boundaries: calls pass by pointer,
/// callees expect by value.
class RemoveByValAttributesPass final
    : public llvm::PassInfoMixin<RemoveByValAttributesPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace host

#endif  // HOST_REMOVE_BYVAL_ATTRIBUTES_PASS_H_INCLUDED
