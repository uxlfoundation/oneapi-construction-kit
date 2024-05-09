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

#ifndef COMPILER_UTILS_STRUCT_TYPE_REMAPPER_H_INCLUDED
#define COMPILER_UTILS_STRUCT_TYPE_REMAPPER_H_INCLUDED

/// @file
///
/// @brief Defines the StructTypeRemapper class for targets to use.

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/Casting.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <multi_llvm/llvm_version.h>
#include <multi_llvm/vector_type_helper.h>

namespace compiler {
namespace utils {

/// @brief Type alias of mapping between struct types.
using StructMap = llvm::DenseMap<llvm::StructType *, llvm::StructType *>;

/// @addtogroup utils
/// @{

/// @brief Remap structs from one type to another
///
/// This is intended to help with the cases where duplicatate opaque types exist
/// between two modules or a module and a context. This can be used to fixup
/// suffixed types remapping based on a user passed map to their unsuffixed
/// verions. More generally it can be used to remap struct types.
class StructTypeRemapper final : public llvm::ValueMapTypeRemapper {
 public:
  /// @brief Callback called when remapping values
  /// @param[in] srcType Current type of Value being cloned
  ///
  /// @return Alternative type if one could be found, existing type otherwise.
  llvm::Type *remapType(llvm::Type *srcType) override {
    if (auto *structType = llvm::dyn_cast<llvm::StructType>(srcType)) {
      auto *newStructType = map.lookup(structType);
      if (newStructType) {
        return newStructType;
      }
    } else if (llvm::isa<llvm::PointerType>(srcType)) {
      return srcType;
    } else if (auto *arrayType = llvm::dyn_cast<llvm::ArrayType>(srcType)) {
      auto *arrayElementType = arrayType->getElementType();
      auto numElements = arrayType->getNumElements();
      return llvm::ArrayType::get(remapType(arrayElementType), numElements);
    } else if (auto *vectorType = llvm::dyn_cast<llvm::VectorType>(srcType)) {
      auto *vectorElementType = vectorType->getElementType();
      auto numElements = multi_llvm::getVectorElementCount(vectorType);
      return llvm::VectorType::get(remapType(vectorElementType), numElements);
    }
    // Note: There may be other derived types we need to handle here.

    return srcType;
  }

  /// @brief Indicates whether type will be remapped.
  ///
  /// @param[in] type Type to query.
  ///
  /// @return Whether type will be remapped.
  /// @retval true if type will remapped, false otherise
  bool isRemapped(llvm::Type *type) { return type != remapType(type); }

  /// @brief Constructor taking a struct map.
  StructTypeRemapper(const StructMap &m) : map(m) {}

 private:
  /// @brief Reference to map between old and new structs
  StructMap map;
};
/// @}
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_STRUCT_TYPE_REMAPPER_H_INCLUDED
