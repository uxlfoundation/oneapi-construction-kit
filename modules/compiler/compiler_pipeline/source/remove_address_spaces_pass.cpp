// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <compiler/utils/remove_address_spaces_pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;

#if LLVM_VERSION_LESS(20, 0)
static Type *removeAddressSpaces(Type *T) {
  constexpr unsigned DefaultAddressSpace = 0;

  if (auto *PtrTy = dyn_cast<PointerType>(T)) {
    if (PtrTy->getAddressSpace() != DefaultAddressSpace) {
      return PointerType::get(T->getContext(), DefaultAddressSpace);
    }
  } else if (auto *VecTy = dyn_cast<VectorType>(T)) {
    if (auto *EltTy = removeAddressSpaces(VecTy->getElementType())) {
      return VectorType::get(EltTy, VecTy);
    }
  }

  return nullptr;
}

PreservedAnalyses compiler::utils::RemoveAddressSpacesPass::run(
    Function &F, FunctionAnalysisManager &) {
  bool Changed = false;

  auto MaybeCastOperand = [&](Instruction &I, Use &Op, Type *T) {
    if (Op->getType() == T) return false;
    IRBuilder<> B(&I);
    Op.set(B.CreateAddrSpaceCast(Op, T));
    return true;
  };

  auto MaybeCastResult = [&](Value &V, Type *OldTy, Type *NewTy,
                             BasicBlock::iterator InsertPt) {
    if (V.use_empty()) return false;
    auto *CastV =
        new AddrSpaceCastInst(PoisonValue::get(OldTy), NewTy, "", InsertPt);
    V.mutateType(NewTy);
    V.replaceAllUsesWith(CastV);
    V.mutateType(OldTy);
    CastV->setOperand(0, &V);
    return true;
  };

  // If any args have a non-default address space, replace their uses with a
  // cast to the default address space inserted in the entry block.
  auto InsertPt = F.getEntryBlock().getFirstNonPHIOrDbgOrAlloca();
  for (auto &Arg : F.args()) {
    if (auto *T = Arg.getType(); auto *NewTy = removeAddressSpaces(T)) {
      Changed |= MaybeCastResult(Arg, T, NewTy, InsertPt);
    }
  }

  // Mutate all instructions to remove non-default address spaces. In most cases
  // this is done by mutating the instruction directly, but in calls and
  // extractvalues we cannot do that so insert a new addrspacecast instruction
  // instead.
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (auto *T = I.getType(); auto *NewTy = removeAddressSpaces(T)) {
        // call and extractvalue instructions need special handling. Cast their
        // result instead.
        if (isa<CallBase>(I) || isa<ExtractValueInst>(I)) {
          InsertPt = std::next(I.getIterator());
          InsertPt.setHeadBit(true);
          Changed |= MaybeCastResult(I, T, NewTy, InsertPt);
        } else {
          I.mutateType(NewTy);
          Changed = true;
        }
      }
    }
  }

  // Now go over instructions again to remove address space casts made
  // redundant, and insert new address space casts as required.
  for (auto &BB : F) {
    for (auto &I : make_early_inc_range(BB)) {
      if (isa<AddrSpaceCastInst>(I)) {
        if (I.getType() == I.getOperand(0)->getType()) {
          I.replaceAllUsesWith(I.getOperand(0));
          I.eraseFromParent();
          Changed = true;
        }
        continue;
      }

      // For call instructions, operand types need to match parameter types.
      if (auto *CB = dyn_cast<CallBase>(&I)) {
        auto *FTy = CB->getFunctionType();
        for (unsigned Idx = 0, E = FTy->getNumParams(); Idx != E; ++Idx) {
          Changed |=
              MaybeCastOperand(I, I.getOperandUse(Idx), FTy->getParamType(Idx));
        }
        continue;
      }

      // For ret instructions, operand types need to match return type.
      if (auto *RI = dyn_cast<ReturnInst>(&I); RI && RI->getReturnValue()) {
        Changed |= MaybeCastOperand(I, I.getOperandUse(0), F.getReturnType());
        continue;
      }

      // For insertvalue instructions, operand types need to match structure or
      // array element type.
      if (auto *IVI = dyn_cast<InsertValueInst>(&I)) {
        Changed |= MaybeCastOperand(
            I, I.getOperandUse(InsertValueInst::getInsertedValueOperandIndex()),
            ExtractValueInst::getIndexedType(
                IVI->getAggregateOperand()->getType(), IVI->getIndices()));
        continue;
      }

      // For other instructions, operands should be non-address-space-qualified.
      // Operands that are arguments or other instructions will have been
      // updated already, but we may still have constants to deal with.
      for (auto &Op : I.operands()) {
        if (auto *T = Op->getType(); auto *NewTy = removeAddressSpaces(T)) {
          Changed |= MaybeCastOperand(I, Op, NewTy);
        }
      }
    }
  }

  if (Changed) {
    PreservedAnalyses PA;
    PA.preserveSet<CFGAnalyses>();
    return PA;
  } else {
    return PreservedAnalyses::all();
  }
}
#endif
