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
/// Align module structs pass.

#ifndef COMPILER_UTILS_ALIGN_MODULE_STRUCTS_PASS_H_INCLUDED
#define COMPILER_UTILS_ALIGN_MODULE_STRUCTS_PASS_H_INCLUDED

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief Pass for padding structs according to OpenCL alignment requirements
/// for the members.
///
/// Creates new padded struct types with correct alignment. Then replaces all
/// references in the module to unpadded struct, with the matching padded
/// variant. Pass can be run conditionally depending on architecture, e.g
/// X86_64 is already correctly aligned while 32 bit ARM and X86 need manual
/// padding from this pass.
class AlignModuleStructsPass
    : public llvm::PassInfoMixin<AlignModuleStructsPass> {
 public:
  /// @brief Container describing new struct type being created.
  ///
  /// Helper class encapsulating all the details needed at the various
  /// stages of creating a new struct type, and how the new members
  /// map to the original ones.
  struct ReplacementStructDetails final {
    /// @brief Initially opaque LLVM type of new struct
    llvm::StructType *newStructType;
    /// @brief Mapping of member indices in old struct to new struct
    llvm::DenseMap<unsigned int, unsigned int> memberIndexMap;
    /// @brief Types of new struct members
    llvm::SmallVector<llvm::Type *, 8> bodyElements;

    /// @brief Constructor for initializing all members.
    ReplacementStructDetails(
        llvm::StructType *structTy,
        llvm::DenseMap<unsigned int, unsigned int> indexMap,
        llvm::SmallVector<llvm::Type *, 8> body)
        : newStructType(structTy),
          memberIndexMap(indexMap),
          bodyElements(body) {}
  };

  using ReplacementStructSP = std::shared_ptr<ReplacementStructDetails>;
  using StructReplacementMap =
      llvm::DenseMap<llvm::Type *, ReplacementStructSP>;

  AlignModuleStructsPass() = default;

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

 private:
  /// @brief Given an unpadded struct type generate a matching padded type
  ///
  /// Target architecture alignment can be different from the member size, so
  /// that alignment is OpenCL conformant we need to manually pad our struct
  /// so that each member meets OpenCL alignment expectations.
  ///
  /// To do this we keep track of each elements offset in the struct,
  /// and ensure that it is a multiple of that elements specified alignment.
  ///
  /// We may also have packed struct types passed in, which although don't need
  /// padding can have struct members which need replaced which our new types.
  ///
  /// @param[in] unpadded struct type we want to create a padded alternative for
  /// @param[in] module LLVM module we're using
  void generateNewStructType(llvm::StructType *unpadded,
                             const llvm::Module &module);

  /// @brief Updates struct member references to other structs to use padded
  ///        variants.
  void fixupStructReferences();

  /// @brief  Map of unpadded struct types to helper class encapsulating details
  ///         of matching padded struct type
  StructReplacementMap originalStructMap;
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_ALIGN_MODULE_STRUCTS_PASS_H_INCLUDED
