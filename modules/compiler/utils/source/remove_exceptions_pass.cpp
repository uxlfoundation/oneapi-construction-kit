// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/remove_exceptions_pass.h>

using namespace llvm;

PreservedAnalyses compiler::utils::RemoveExceptionsPass::run(
    Function &F, FunctionAnalysisManager &) {
  // We don't use exceptions
  // Adding this attribute here is the "nuclear option", it would be better
  // to ensure it is added at source but it is not always plausible to do.
  if (!F.hasFnAttribute(llvm::Attribute::NoUnwind)) {
    F.addFnAttr(llvm::Attribute::NoUnwind);
  }

  return PreservedAnalyses::all();
}
