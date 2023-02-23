// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief LLVM helper methods.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_LLVM_HELPERS_H_INCLUDED
#define VECZ_LLVM_HELPERS_H_INCLUDED

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/IRBuilder.h>
#include <multi_llvm/llvm_version.h>
#include <multi_llvm/vector_type_helper.h>

namespace vecz {

/// @brief Determine if the value has vector type, and return it.
///
/// @param[in] V Value to analyze.
///
/// @return Vector type of V or null.
multi_llvm::FixedVectorType *getVectorType(llvm::Value *V);

/// @brief Get the default value for a type.
///
/// @param[in] T Type to get default value of.
/// @param[in] V Default value to use for numeric type
///
/// @return Default value, which will be undef for non-numeric types
llvm::Value *getDefaultValue(llvm::Type *T, uint64_t V = 0UL);

/// @brief Get the shuffle mask as sequence of integers.
///
/// @param[in] Shuffle Instruction
///
/// @return Array of integers representing the Shuffle mask
llvm::ArrayRef<int> getShuffleVecMask(llvm::ShuffleVectorInst *Shuffle);
}  // namespace vecz

#endif  // VECZ_LLVM_HELPERS_H_INCLUDED
