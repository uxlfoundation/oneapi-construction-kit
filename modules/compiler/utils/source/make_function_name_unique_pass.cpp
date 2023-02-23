// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/attributes.h>
#include <compiler/utils/make_function_name_unique_pass.h>

using namespace llvm;

PreservedAnalyses compiler::utils::MakeFunctionNameUniquePass::run(
    Function &F, FunctionAnalysisManager &) {
  if (isKernelEntryPt(F)) {
    F.setName(UniqueName);
  }

  return PreservedAnalyses::all();
}
