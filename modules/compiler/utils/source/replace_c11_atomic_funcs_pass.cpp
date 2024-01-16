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

/// @file
///
/// @brief Implements the minimal subset of C11 Atomics required by OpenCL-3.0.

#include <compiler/utils/metadata.h>
#include <compiler/utils/replace_c11_atomic_funcs_pass.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Pass.h>
#include <llvm/Support/Debug.h>

// For debug output when --debug-only=replace_c11_atomics
#define DEBUG_TYPE "replace_c11_atomics"
STATISTIC(NumReplacedCalls, "Number of C11 Atomic calls replaced");

using namespace llvm;

namespace {
/// @brief Helper function for debug output.
///
/// Prints to dbgs() the before and after instructions and increments the
/// statistic NumReplacedCalls which counts the number of replaced
/// instructions.
///
/// @param[in] Old Instruction being replaced.
/// @param[in] New Instruction replacing with.
void debugOutput(CallInst *Old, Instruction *New) {
  // This is required to stop release builds from complaining about unused
  // variables.
  (void)Old;
  (void)New;
  LLVM_DEBUG(dbgs() << "Replaced: "; Old->print(dbgs(), true);
             dbgs() << "\n with: "; New->print(dbgs(), true); dbgs() << "\n");
  ++NumReplacedCalls;
}
/// @brief Replaces an atomic_init instruction.
///
/// Replaces an atomic_init instruction with a store instruction.
///
/// @param[in] Store atomic_init instruction to replace
void replaceInit(CallInst *C11Init) {
  auto *Obj = C11Init->getOperand(0);
  auto *Value = C11Init->getOperand(1);

  IRBuilder<> Builder{C11Init};
  auto *Store = Builder.CreateStore(Value, Obj, true);
  debugOutput(C11Init, Store);

  // Update and remove the original call.
  C11Init->eraseFromParent();
}

/// @brief Replaces an atomic_store_explicit instruction.
///
/// Replaces an atomic_store_explicit instruction with an atomic store
/// instruction.
///
/// @param[in] Store atomic_store instruction to replace
void replaceStore(CallInst *C11Store) {
  auto *Object = C11Store->getOperand(0);
  auto *Desired = C11Store->getOperand(1);

  IRBuilder<> Builder{C11Store};
  auto *AtomicStore = Builder.CreateStore(Desired, Object, true);
  AtomicStore->setAtomic(AtomicOrdering::Monotonic);
  const uint64_t StoreAlignmentSizeInBytes{
      C11Store->getOperand(1)->getType()->getScalarSizeInBits() >> 3};
  auto Alignment = MaybeAlign(StoreAlignmentSizeInBytes).valueOrOne();
  AtomicStore->setAlignment(Alignment);
  debugOutput(C11Store, AtomicStore);

  // Update and remove the original call.
  C11Store->eraseFromParent();
}

/// @brief Replaces an atomic_store_explicit instruction.
///
/// Replaces an atomic_store_explicit instruction with an atomic store
/// instruction.
///
/// @param[in] Store atomic_store instruction to replace
void replaceLoad(CallInst *C11Load) {
  auto *Object = C11Load->getOperand(0);

  IRBuilder<> Builder{C11Load};
  auto *AtomicLoad = Builder.CreateLoad(C11Load->getType(), Object);
  AtomicLoad->setAtomic(AtomicOrdering::Monotonic);
  const uint64_t LoadAlignmentSizeInBytes{
      C11Load->getType()->getScalarSizeInBits() >> 3};
  auto Alignment = MaybeAlign(LoadAlignmentSizeInBytes).valueOrOne();
  AtomicLoad->setAlignment(Alignment);
  debugOutput(C11Load, AtomicLoad);

  // Update and remove the original call.
  C11Load->replaceAllUsesWith(AtomicLoad);
  C11Load->eraseFromParent();
}

/// @brief Replaces an atomic_exchange_explicit builtin call.
///
/// Replaces an atomic_exchange_explicit call with an atomic instruction
///
/// @param[in] C11Exchange atomic_exchange_explicit call to replace.
void replaceExchange(CallInst *C11Exchange) {
  auto *Object = C11Exchange->getOperand(0);
  auto *Desired = C11Exchange->getOperand(1);

  IRBuilder<> Builder{C11Exchange};
  auto *AtomicExchange = Builder.CreateAtomicRMW(
      AtomicRMWInst::Xchg, Object, Desired, MaybeAlign(),
      AtomicOrdering::Monotonic, SyncScope::System);
  debugOutput(C11Exchange, AtomicExchange);
  // Update and remove the original call.
  C11Exchange->replaceAllUsesWith(AtomicExchange);
  C11Exchange->eraseFromParent();
}

/// @brief Replaces an atomic_compare_exchange_(strong|weak)_explicit builtin
/// call.
///
/// Replaces an atomic_compare_(strong|weak)_exchange call with an atomic
/// instruction.
///
/// @param[in] C11CompareExchange
/// atomic_compare_exchange_(strong|weak)_explicit call to replace.
void implementCompareExchange(Function *C11CompareExchangeFunc, bool IsWeak) {
  auto *Object = C11CompareExchangeFunc->getArg(0);
  auto *Expected = C11CompareExchangeFunc->getArg(1);
  auto *Desired = C11CompareExchangeFunc->getArg(2);

  // The semantics of the C11 atomic compare exchange and LLVM's atomic
  // compare exchange are slightly different:
  //
  // Firstly, C11 Atomics expected argument is a pointer, where as LLVM's is a
  // register, so we need to wrap the instruction in a load and a store pair.
  //
  // Secondly, OpenCL cmpxch is equivilent to:
  // if (memcmp(object, expected, sizeof(*object) == 0)
  //     memcpy(object, &desired, sizeof(*object));
  // else
  //     memcpy(expected, object, sizeof(*object));
  //
  // where as LLVM is only:
  // if (memcmp(object, expected, sizeof(*object) == 0)
  //     memcpy(object, &desired, sizeof(*object));
  //
  // So we need to branch based on the result of the instruction.
  auto &Ctx = C11CompareExchangeFunc->getContext();
  auto *ExitBB = BasicBlock::Create(Ctx, "exit", C11CompareExchangeFunc);
  auto *FailureBB =
      BasicBlock::Create(Ctx, "failure", C11CompareExchangeFunc, ExitBB);
  auto *EntryBB =
      BasicBlock::Create(Ctx, "entry", C11CompareExchangeFunc, FailureBB);

  IRBuilder<> EntryBBBuilder{EntryBB};
  auto *LoadExpected = EntryBBBuilder.CreateLoad(Desired->getType(), Expected);
  auto *AtomicCompareExchange = EntryBBBuilder.CreateAtomicCmpXchg(
      Object, LoadExpected, Desired, MaybeAlign(), AtomicOrdering::Monotonic,
      AtomicOrdering::Monotonic, SyncScope::System);
  // The default semantics are strong.
  AtomicCompareExchange->setWeak(IsWeak);
  auto *Success = EntryBBBuilder.CreateExtractValue(AtomicCompareExchange, 1);
  auto *OriginalValue =
      EntryBBBuilder.CreateExtractValue(AtomicCompareExchange, 0);
  EntryBBBuilder.CreateCondBr(Success, ExitBB, FailureBB);

  IRBuilder<> FailureBBBuilder{FailureBB};
  FailureBBBuilder.CreateStore(OriginalValue, Expected);
  FailureBBBuilder.CreateBr(ExitBB);

  IRBuilder<> ExitBBBuilder{ExitBB};
  auto *CastedResult = ExitBBBuilder.CreateIntCast(
      Success, C11CompareExchangeFunc->getReturnType(), false);
  ExitBBBuilder.CreateRet(CastedResult);
}

/// @brief Wrapper for the implementCompareExchange function implementing the
/// atomic_compare_exchange_strong_explicit builtin.
///
/// Prints debug info, then calls into the implementCompareExchange function
/// to implement the atomic_compare_exchange_strong_explicit builtin.
///
/// @param[in] C11CompareExchangeStrongFunc atomic_compare_exchange_strong
/// declaration to be implemented.
void implementCompareExchangeStrong(Function *C11CompareExchangeStrongFunc) {
  LLVM_DEBUG(dbgs() << "Implementing the "
                       "atomic_compare_exchange_strong_explicit builtin\n");
  implementCompareExchange(C11CompareExchangeStrongFunc, /* IsWeak */ false);
}

/// @brief Wrapper for the implementCompareExchange function implementing the
/// atomic_compare_exchange_weak_explicit builtin.
///
/// Prints debug info, then calls into the implementCompareExchange function
/// to implement the atomic_compare_exchange_weak_explicit builtin.
///
/// @param[in] C11CompareExchangeweakFunc atomic_compare_exchange_weak
/// declaration to be implemented.
void implementCompareExchangeWeak(Function *C11CompareExchangeWeakFunc) {
  LLVM_DEBUG(dbgs() << "Implementing the "
                       "atomic_compare_exchange_weak_explicit builtin\n");
  implementCompareExchange(C11CompareExchangeWeakFunc, /* IsWeak */ true);
}

/// @brief Replaces an atomic_fetch_key builtin call.
///
/// Replaces an atomic_fetch_key call with an atomic
/// instruction where key is in {add, sub, or, xor, and, min max}.
///
/// @param[in] C11AtomicFetch atomic_fetch_key call to replace.
void replaceFetchKey(CallInst *C11FetchKey) {
  auto *Object = C11FetchKey->getOperand(0);
  auto *Operand = C11FetchKey->getOperand(1);

  // Since all the atomic_fetch_key buitins have the same signature we can
  // handle them all at once if we switch on the key.
  auto BuiltinName = C11FetchKey->getCalledFunction()->getName();
  auto KeyStartIndex =
      BuiltinName.find("atomic_fetch_") + std::strlen("atomic_fetch_");
  auto KeyEndIndex = BuiltinName.find('_', KeyStartIndex);
  auto Key = BuiltinName.substr(KeyStartIndex, KeyEndIndex - KeyStartIndex);

  const bool IsFloat = C11FetchKey->getType()->isFloatingPointTy();
  AtomicRMWInst::BinOp KeyOpCode{IsFloat
                                     ? StringSwitch<AtomicRMWInst::BinOp>(Key)
                                           .Case("add", AtomicRMWInst::FAdd)
                                           .Case("min", AtomicRMWInst::FMin)
                                           .Case("max", AtomicRMWInst::FMax)
                                     : StringSwitch<AtomicRMWInst::BinOp>(Key)
                                           .Case("add", AtomicRMWInst::Add)
                                           .Case("sub", AtomicRMWInst::Sub)
                                           .Case("or", AtomicRMWInst::Or)
                                           .Case("xor", AtomicRMWInst::Xor)
                                           .Case("and", AtomicRMWInst::And)
                                           .Case("min", AtomicRMWInst::Min)
                                           .Case("max", AtomicRMWInst::Max)};

  // We need to make sure we distinguish between signed and unsigned integer
  // comparisons for min and max since they are different operations in
  // unsigned and two's compliment arithmetic.
  //
  // We can check this by looking at the name mangling i = signed, j =
  // unsigned. This is brittle but given that the name mangling looks like:
  // _Z25atomic_fetch_min_explicitPU3AS3VU7_Atomicii12memory_order12memory_scope
  // we just look for "Atomic"
  if (AtomicRMWInst::Min == KeyOpCode || AtomicRMWInst::Max == KeyOpCode) {
    auto OpTypeMangledIndex =
        BuiltinName.find("Atomic", KeyEndIndex) + std::strlen("Atomic");
    switch (BuiltinName[OpTypeMangledIndex]) {
      case 'j':
      case 'm':
        KeyOpCode = (KeyOpCode == AtomicRMWInst::Min) ? AtomicRMWInst::UMin
                                                      : AtomicRMWInst::UMax;
    }
  }

  IRBuilder<> Builder{C11FetchKey};
  auto *AtomicFetchKey =
      Builder.CreateAtomicRMW(KeyOpCode, Object, Operand, MaybeAlign(),
                              AtomicOrdering::Monotonic, SyncScope::System);
  debugOutput(C11FetchKey, AtomicFetchKey);
  // Update and remove the original call.
  C11FetchKey->replaceAllUsesWith(AtomicFetchKey);
  C11FetchKey->eraseFromParent();
}

/// @brief Replaces an atomic_flag_test_and_set_explicit builtin call.
///
/// Replaces an atomic_flag_test_and_set_explicit call with an atomic
/// instruction.
///
/// @param[in] C11FlagTestAndSet atomic_fetch_key call to replace.
void replaceFlagTestAndSet(CallInst *C11FlagTestAndSet) {
  auto *Object = C11FlagTestAndSet->getOperand(0);

  IRBuilder<> Builder{C11FlagTestAndSet};
  // OpenCL spec 6.15.12.6:
  // The atomic_flag type must be implemented as a 32-bit integer
  auto *TrueValue = Builder.getInt32(1);
  auto *AtomicFlagTestAndSet = Builder.CreateAtomicRMW(
      AtomicRMWInst::Xchg, Object, TrueValue, MaybeAlign(),
      AtomicOrdering::Monotonic, SyncScope::System);
  auto *CastedResult = Builder.CreateIntCast(
      AtomicFlagTestAndSet, Type::getInt1Ty(C11FlagTestAndSet->getContext()),
      false);
  debugOutput(C11FlagTestAndSet, AtomicFlagTestAndSet);

  // Update and remove original call.
  C11FlagTestAndSet->replaceAllUsesWith(CastedResult);
  C11FlagTestAndSet->eraseFromParent();
}

/// @brief Replaces an atomic_flag_clear_explicit builtin call.
///
/// Replaces an atomic_flag_clear_explicit call with an atomic
/// instruction.
///
/// @param[in] C11FlagClear atomic_fetch_key call to replace.
void replaceFlagClear(CallInst *C11FlagClear) {
  auto *Object = C11FlagClear->getOperand(0);

  IRBuilder<> Builder{C11FlagClear};
  // OpenCL spec 6.15.12.6:
  // The atomic_flag type must be implemented as a 32-bit integer
  auto *FalseValue = Builder.getInt32(0);
  auto *AtomicFlagTestAndSet = Builder.CreateAtomicRMW(
      AtomicRMWInst::Xchg, Object, FalseValue, MaybeAlign(),
      AtomicOrdering::Monotonic, SyncScope::System);
  debugOutput(C11FlagClear, AtomicFlagTestAndSet);

  // Update and remove original call.
  C11FlagClear->eraseFromParent();
}

/// @brief Change function call for atomic to instruction.
///
/// @param[in,out] call Call instruction for transformation.
///
/// @return bool Whether or not the pass changed anything.
bool runOnInstruction(CallInst *Call) {
  if (auto *Callee = Call->getCalledFunction()) {
    auto Name{Callee->getName()};
    if (Name.contains("_Z11atomic_init")) {
      replaceInit(Call);
      return true;
    } else if (Name.contains("_Z20atomic_load_explicit")) {
      replaceLoad(Call);
      return true;
    } else if (Name.contains("_Z21atomic_store_explicit")) {
      replaceStore(Call);
      return true;
    } else if (Name.contains("_Z24atomic_exchange_explicit")) {
      replaceExchange(Call);
      return true;
    } else if (Name.contains("atomic_fetch_")) {
      replaceFetchKey(Call);
      return true;
    } else if (Name.contains("_Z33atomic_flag_test_and_set_explicit")) {
      replaceFlagTestAndSet(Call);
      return true;
    } else if (Name.contains("_Z26atomic_flag_clear_explicit")) {
      replaceFlagClear(Call);
      return true;
    }
  }
  return false;
}

/// @brief Iterate instructions.
///
/// @param[in,out] block Basic block for checking.
///
/// @return Return whether or not the pass changed anything.
bool runOnBasicBlock(BasicBlock &Block) {
  auto Result{false};
  // Here we use a 'while' loop instead of a 'for' loop to avoid
  // invalidating the iterator when removing instructions.
  auto Iter = Block.begin();
  while (Iter != Block.end()) {
    auto *Inst = &*(Iter++);
    if (auto *Call = llvm::dyn_cast<llvm::CallInst>(Inst)) {
      Result |= runOnInstruction(Call);
    }
  }
  return Result;
}

/// @brief Iterate basic blocks.
///
/// @param[in,out] function Function for checking.
///
/// @return Whether or not the pass changed anything.
bool runOnFunction(llvm::Function &Function) {
  auto Result{false};
  for (auto &BasicBlock : Function) {
    Result |= runOnBasicBlock(BasicBlock);
  }
  return Result;
}

}  // namespace

/// @brief The entry point to the pass.
///
/// Overrides llvm::ModulePass::runOnModule().
///
/// @param[in,out] M - The module provided to the pass.
///
/// @return Return whether or not the pass changed anything.
PreservedAnalyses compiler::utils::ReplaceC11AtomicFuncsPass::run(
    Module &M, ModuleAnalysisManager &) {
  // Only run this pass for OpenCL 2.0+ modules.
  // FIXME: This would be better off inside BuiltinInfo, and combined with the
  // regular ReplaceAtomicFuncsPass.
  auto version = getOpenCLVersion(M);
  if (version < OpenCLC20) {
    return PreservedAnalyses::all();
  }
  auto Changed{false};
  for (auto &Function : M) {
    // First see if the function is one of the special cases. Any builtin that
    // takes more than one instruction to implement we create a function body
    // for the call.
    if (Function.getName().contains(
            "_Z37atomic_compare_exchange_weak_explicit")) {
      implementCompareExchangeWeak(&Function);
      Changed = true;
      continue;
    }

    if (Function.getName().contains(
            "_Z39atomic_compare_exchange_strong_explicit")) {
      implementCompareExchangeStrong(&Function);
      Changed = true;
      continue;
    }

    // Otherwise we just replace the call with a single atomic instruction.
    Changed |= runOnFunction(Function);
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

#undef DEBUG_TYPE
