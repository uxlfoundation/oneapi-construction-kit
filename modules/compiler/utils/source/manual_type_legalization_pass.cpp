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
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Casting.h>
#include <llvm/TargetParser/Triple.h>
#include <multi_llvm/llvm_version.h>

using namespace llvm;

PreservedAnalyses compiler::utils::ManualTypeLegalizationPass::run(
    Function &F, FunctionAnalysisManager &FAM) {
  auto &C = F.getContext();
  auto *HalfT = Type::getHalfTy(C);
  auto *FloatT = Type::getFloatTy(C);
  auto *DoubleT = Type::getDoubleTy(C);

  // Targets where half is a legal type, and targets where half is promoted
  // using "soft promotion" rules, are assumed to implement basic operators
  // correctly. We cannot reliably determine which targets use "soft promotion"
  // rules so we hardcode the list here.
  //
  // FMA is promoted incorrectly on all targets without hardware support, even
  // when using "soft promotion" rules; only targets that have native support
  // implement it correctly at the moment.
  //
  // Both for operators and FMA, whether the target implements the operation
  // correctly may depend on the target feature string. We ignore that here for
  // simplicity.
  const llvm::Triple TT(F.getParent()->getTargetTriple());

  auto &TTI = FAM.getResult<TargetIRAnalysis>(F);
  const bool HaveCorrectHalfOps = TTI.isTypeLegal(HalfT) ||
#if LLVM_VERSION_GREATER_EQUAL(19, 0)
                                  TT.isARM() ||
#endif
                                  TT.isX86() || TT.isRISCV();
  const bool HaveCorrectHalfFMA = TT.isRISCV();
  const bool HaveCorrectVecConvert = !TT.isAArch64();
  if (HaveCorrectHalfOps && HaveCorrectHalfFMA && HaveCorrectVecConvert) {
    return PreservedAnalyses::all();
  }

  DenseMap<std::pair<Value *, Type *>, Value *> FPExtVals;
  IRBuilder<> B(C);

  auto CreateFPExt = [&](Value *V, Type *Ty, Type *ExtTy) {
    (void)Ty;
    assert(V->getType() == Ty &&
           "Expected matching types for floating point operation");
    auto *&FPExt = FPExtVals[{V, ExtTy}];
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
      auto *T = I.getType();
      auto *VecT = dyn_cast<VectorType>(T);
      auto *ElT = VecT ? VecT->getElementType() : T;

      if (ElT == HalfT) {
        if (!HaveCorrectHalfOps) {
          if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
            Type *const ExtElT = FloatT;
            Type *const ExtT =
                VecT ? VectorType::get(ExtElT, VecT->getElementCount())
                     : ExtElT;
            Value *const PromotedOperands[] = {
                CreateFPExt(BO->getOperand(0), T, ExtT),
                CreateFPExt(BO->getOperand(1), T, ExtT),
            };
            B.SetInsertPoint(BO);
            B.setFastMathFlags(BO->getFastMathFlags());
            auto *const PromotedOperation =
                B.CreateBinOp(BO->getOpcode(), PromotedOperands[0],
                              PromotedOperands[1], BO->getName() + ".fpext");
            B.clearFastMathFlags();

            auto *const Trunc = B.CreateFPTrunc(PromotedOperation, T);
            Trunc->takeName(BO);

            BO->replaceAllUsesWith(Trunc);
            BO->eraseFromParent();

            Changed = true;
            continue;
          }
        }

        if (!HaveCorrectHalfFMA) {
          if (auto *II = dyn_cast<IntrinsicInst>(&I)) {
            if (II->getIntrinsicID() == Intrinsic::fma) {
              Type *const ExtElT = DoubleT;
              Type *const ExtT =
                  VecT ? VectorType::get(ExtElT, VecT->getElementCount())
                       : ExtElT;
              Value *const PromotedArguments[] = {
                  CreateFPExt(II->getArgOperand(0), T, ExtT),
                  CreateFPExt(II->getArgOperand(1), T, ExtT),
                  CreateFPExt(II->getArgOperand(2), T, ExtT),
              };
              B.SetInsertPoint(II);
              // Because the arguments are promoted halfs, the multiplication in
              // type double is exact and the result is the same even if
              // multiply and add are kept as separate operations, so use
              // FMulAdd rather than FMA.
              auto *const PromotedOperation =
                  B.CreateIntrinsic(ExtT, Intrinsic::fmuladd, PromotedArguments,
                                    II, II->getName() + ".fpext");

              auto *const Trunc = B.CreateFPTrunc(PromotedOperation, T);
              Trunc->takeName(II);

              II->replaceAllUsesWith(Trunc);
              II->eraseFromParent();

              Changed = true;
              continue;
            }
          }
        }
      }

      if (!HaveCorrectVecConvert) {
        if (I.getOpcode() == Instruction::UIToFP ||
            I.getOpcode() == Instruction::SIToFP) {
          auto &CI = cast<CastInst>(I);
          auto *const Op = CI.getOperand(0);
          if (CI.getType()->isVectorTy() &&
              CI.getType()->getScalarSizeInBits() <
                  Op->getType()->getScalarSizeInBits()) {
            auto *const VecTy = cast<FixedVectorType>(CI.getType());
            auto *const EltTy = VecTy->getElementType();

            Value *Replacement = PoisonValue::get(VecTy);
            B.SetInsertPoint(&CI);
            for (unsigned I = 0, E = VecTy->getNumElements(); I != E; ++I) {
              auto *const EEI = B.CreateExtractElement(Op, I);
              auto *const CastI = B.CreateCast(CI.getOpcode(), EEI, EltTy);
              auto *const IEI = B.CreateInsertElement(Replacement, CastI, I);
              Replacement = IEI;
            }

            CI.replaceAllUsesWith(Replacement);
            CI.eraseFromParent();

            Changed = true;
            continue;
          }
        }
      }
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
