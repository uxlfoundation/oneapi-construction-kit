// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/builtin_info.h>
#include <compiler/utils/encode_builtin_range_metadata_pass.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;

PreservedAnalyses compiler::utils::EncodeBuiltinRangeMetadataPass::run(
    Module &M, ModuleAnalysisManager &AM) {
  auto &Context = M.getContext();
  auto &BI = AM.getResult<BuiltinInfoAnalysis>(M);

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto *CI = dyn_cast<CallInst>(&I);
        if (!CI) {
          continue;
        }
        auto *RetTy = CI->getType();
        // Range metadata only applies to integer-typed functions. Don't bother
        // proceeding if the function returns anything else.
        if (!RetTy->isIntegerTy()) {
          continue;
        }
        // If there's already range metadata, assume it's more accurate than
        // what we're about to apply (this lets users apply their own ranges
        // first if they wish).
        if (CI->getMetadata(LLVMContext::MD_range)) {
          continue;
        }

        auto Range = BI.getBuiltinRange(*CI, MaxLocalSizes, MaxGlobalSizes);

        // If no range has been computed, or it's the trivial full set of
        // values, don't bother setting metadata.
        if (!Range || Range->isFullSet()) {
          continue;
        }
        // Set a single contiguous !range metadata [min,max).
        Metadata *LowAndHigh[] = {
            ConstantAsMetadata::get(ConstantInt::get(RetTy, Range->getLower())),
            ConstantAsMetadata::get(
                ConstantInt::get(RetTy, Range->getUpper()))};
        CI->setMetadata(LLVMContext::MD_range,
                        MDNode::get(Context, LowAndHigh));
      }
    }
  }

  return PreservedAnalyses::all();
}
