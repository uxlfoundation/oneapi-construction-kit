// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <compiler/utils/manual_type_legalization_pass.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Casting.h>
#include <llvm/TargetParser/Triple.h>
#include <multi_llvm/llvm_version.h>

using namespace llvm;

PreservedAnalyses compiler::utils::ManualTypeLegalizationPass::run(
    Function &F, FunctionAnalysisManager &FAM) {
  auto &TTI = FAM.getResult<TargetIRAnalysis>(F);

  auto *HalfT = Type::getHalfTy(F.getContext());
  auto *FloatT = Type::getFloatTy(F.getContext());

  // Targets where half is a legal type do not need this pass. Targets where
  // half is promoted using "soft promotion" rules also do not need this pass.
  // We cannot reliably determine which targets these are, but that is okay, on
  // targets where this pass is not needed it does no harm, it merely wastes
  // time.
  const llvm::Triple TT(F.getParent()->getTargetTriple());
  if (TTI.isTypeLegal(HalfT) || TT.isX86() || TT.isRISCV()) {
    return PreservedAnalyses::all();
  }

  DenseMap<Value *, Value *> FPExtVals;
  IRBuilder<> B(F.getContext());

  auto CreateFPExt = [&](Value *V, Type *ExtTy) {
    auto *&FPExt = FPExtVals[V];
    if (!FPExt) {
      if (auto *I = dyn_cast<Instruction>(V)) {
#if LLVM_VERSION_GREATER_EQUAL(18, 0)
        std::optional<BasicBlock::iterator> IPAD;
        IPAD = I->getInsertionPointAfterDef();
#else
        std::optional<Instruction *> IPAD;
        if (auto *IPADRaw = I->getInsertionPointAfterDef()) {
          IPAD = IPADRaw;
        }
#endif
        assert(IPAD &&
               "getInsertionPointAfterDef() should return an insertion point "
               "for all FP16 instructions");
        B.SetInsertPoint(*IPAD);
      } else {
        B.SetInsertPointPastAllocas(&F);
      }
      FPExt = B.CreateFPExt(V, ExtTy, V->getName() + ".fpext");
    }
    return FPExt;
  };

  bool Changed = false;

  for (auto &BB : F) {
    for (auto &I : make_early_inc_range(BB)) {
      auto *BO = dyn_cast<BinaryOperator>(&I);
      if (!BO) continue;

      auto *T = BO->getType();
      auto *VecT = dyn_cast<VectorType>(T);
      auto *ElT = VecT ? VecT->getElementType() : T;

      if (ElT != HalfT) continue;

      auto *LHS = BO->getOperand(0);
      auto *RHS = BO->getOperand(1);
      assert(LHS->getType() == T &&
             "Expected matching types for floating point operation");
      assert(RHS->getType() == T &&
             "Expected matching types for floating point operation");

      auto *ExtElT = FloatT;
      auto *ExtT =
          VecT ? VectorType::get(ExtElT, VecT->getElementCount()) : ExtElT;

      auto *LHSExt = CreateFPExt(LHS, ExtT);
      auto *RHSExt = CreateFPExt(RHS, ExtT);

      B.SetInsertPoint(BO);

      B.setFastMathFlags(BO->getFastMathFlags());
      auto *OpExt = B.CreateBinOp(BO->getOpcode(), LHSExt, RHSExt,
                                  BO->getName() + ".fpext");
      B.clearFastMathFlags();

      auto *Trunc = B.CreateFPTrunc(OpExt, T);
      Trunc->takeName(BO);

      BO->replaceAllUsesWith(Trunc);
      BO->eraseFromParent();

      Changed = true;
    }
  }

  PreservedAnalyses PA;
  if (Changed) {
    PA = PreservedAnalyses::none();
    PA.preserveSet<CFGAnalyses>();
  } else {
    PA = PreservedAnalyses::all();
  }
  return PA;
}
