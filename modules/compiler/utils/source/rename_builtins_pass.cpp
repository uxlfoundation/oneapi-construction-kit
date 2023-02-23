// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/rename_builtins_pass.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>

llvm::PreservedAnalyses compiler::utils::RenameBuiltinsPass::run(
    llvm::Module &M, llvm::ModuleAnalysisManager &) {
  bool Changed = false;
  constexpr llvm::StringLiteral CoreFnPrefix = "__core";
  constexpr llvm::StringLiteral MuxFnPrefix = "__mux";

  for (auto &fn : M.functions()) {
    if (!fn.getName().startswith(MuxFnPrefix)) {
      continue;
    }
    Changed = true;
    const llvm::StringRef OldFnName = fn.getName();
    const llvm::StringRef BaseFnName =
        OldFnName.substr(MuxFnPrefix.size(), OldFnName.size());
    const llvm::Twine NewFnName = CoreFnPrefix + BaseFnName;
    fn.setName(NewFnName);
  }
  return Changed ? llvm::PreservedAnalyses::all()
                 : llvm::PreservedAnalyses::none();
}
