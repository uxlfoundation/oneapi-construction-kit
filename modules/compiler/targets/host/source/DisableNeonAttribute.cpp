// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <host/disable_neon_attribute_pass.h>
#include <llvm/ADT/Triple.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <multi_llvm/vector_type_helper.h>

using namespace llvm;

// Set attribute disabling Neon on all functions.
// We need to do this for every functions since '-neon' affects the ABI
// calling convention. Currently LLVM(4.0) doesn't account for this
// inconsistency between callee & caller which leads to incorrect results.
//
// Currently this solution isn't performant and will be reviewed in the
// future, see JIRA CA-343.
static PreservedAnalyses disableNEONAttr(Module &M) {
  for (Function &F : M) {
    F.addFnAttr("target-features", "-neon");
  }

  return PreservedAnalyses::all();
}

PreservedAnalyses host::DisableNeonAttributePass::run(Module &M,
                                                      ModuleAnalysisManager &) {
  // This pass is a workaround for an issue specific to 64-bit ARM. Where Neon
  // vector conversions from i64 -> float do the conversion in two stages,
  // i64 -> double then double -> float. This loses precision because of
  // incorrect rounding in the intermediate value.
  Triple triple(M.getTargetTriple());
  if (Triple::aarch64 != triple.getArch()) {
    return PreservedAnalyses::all();
  }

  // Find UIToFP/SIToFP vector cast instructions, casting [u]long -> float.
  for (Function &F : M) {
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        if (auto *cast = dyn_cast<CastInst>(&I)) {
          // Check this cast is an int to float cast
          auto opCode = cast->getOpcode();
          if (opCode != Instruction::UIToFP && opCode != Instruction::SIToFP) {
            continue;
          }

          // Check the cast has vector operand types and is i64 -> float
          auto *vecDstType =
              dyn_cast<multi_llvm::FixedVectorType>(cast->getDestTy());
          auto *vecSrcType =
              dyn_cast<multi_llvm::FixedVectorType>(cast->getSrcTy());
          if (vecDstType && vecDstType->getElementType()->isFloatTy() &&
              vecSrcType && vecSrcType->getElementType()->isIntegerTy(64)) {
            return disableNEONAttr(M);
          }
        }
      }
    }
  }

  // No cast found, exit without disabling Neon
  return PreservedAnalyses::all();
}
