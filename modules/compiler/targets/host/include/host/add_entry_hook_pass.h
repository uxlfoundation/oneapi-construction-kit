// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Add entry hook pass
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef HOST_ADD_ENTRY_HOOK_PASS_H_INCLUDED
#define HOST_ADD_ENTRY_HOOK_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

class AddEntryHookPass final : public llvm::PassInfoMixin<AddEntryHookPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};

#endif  // HOST_ADD_ENTRY_HOOK_PASS_H_INCLUDED
