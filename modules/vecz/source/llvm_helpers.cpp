// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "llvm_helpers.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Module.h>
#include <multi_llvm/multi_llvm.h>

#include "debugging.h"
#include "memory_operations.h"

using namespace llvm;

/// @brief Determine if the value has vector type, and return it.
///
/// @param[in] V Value to analyze.
///
/// @return Vector type of V or null.
multi_llvm::FixedVectorType *vecz::getVectorType(Value *V) {
  if (StoreInst *Store = dyn_cast<StoreInst>(V)) {
    auto *VO = Store->getValueOperand();
    assert(VO && "Could not get value operand");
    return dyn_cast<multi_llvm::FixedVectorType>(VO->getType());
  } else if (CallInst *Call = dyn_cast<CallInst>(V)) {
    if (auto MaskedOp = MemOp::get(Call, MemOpAccessKind::Masked)) {
      if (MaskedOp->isMaskedMemOp() && MaskedOp->isStore()) {
        return dyn_cast<multi_llvm::FixedVectorType>(MaskedOp->getDataType());
      }
    }
  }
  return dyn_cast<multi_llvm::FixedVectorType>(V->getType());
}

/// @brief Get the default value for a type.
///
/// @param[in] T Type to get default value of.
/// @param[in] V Default value to use for numeric type
///
/// @return Default value, which will be undef for non-numeric types
Value *vecz::getDefaultValue(Type *T, uint64_t V) {
  if (T->isIntegerTy()) {
    return ConstantInt::get(T, V);
  }

  if (T->isFloatTy() || T->isDoubleTy()) {
    return ConstantFP::get(T, V);
  }

  return UndefValue::get(T);
}

/// @brief Get the shuffle mask as sequence of integers.
///
/// @param[in] Shuffle Instruction
///
/// @return Array of integers representing the Shuffle mask
ArrayRef<int> vecz::getShuffleVecMask(ShuffleVectorInst *Shuffle) {
  return Shuffle->getShuffleMask();
}
