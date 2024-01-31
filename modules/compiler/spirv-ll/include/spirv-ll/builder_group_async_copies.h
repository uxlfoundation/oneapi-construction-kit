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

#ifndef SPIRV_LL_SPV_BUILDER_GROUP_ASYNC_COPIES_H_INCLUDED
#define SPIRV_LL_SPV_BUILDER_GROUP_ASYNC_COPIES_H_INCLUDED

#include <spirv-ll/builder.h>

namespace spirv_ll {

/// @brief builder for the Codeplay.GroupAsyncCopies extended instruction set.
class GroupAsyncCopiesBuilder : public ExtInstSetHandler {
 public:
  /// @brief Constructor.
  ///
  /// @param[in] builder spirv_ll::Builder object that will own this object.
  /// @param[in] module The module being translated.
  GroupAsyncCopiesBuilder(Builder &builder, Module &module)
      : ExtInstSetHandler(builder, module) {}

  /// @brief Enumeration of instruction ID's in the Codeplay.GroupAsyncCopies
  /// extended instruction set.
  // TODO(CA-4119): If this vendor extension lives beyond any official Khronos
  // extension, these definitions should be defined upstream in SPIRV-Headers.
  enum Instruction {
    GroupAsyncCopy2D2D = 1,  ///< Represents async_work_group_copy_2D2D.
    GroupAsyncCopy3D3D = 2,  ///< Represents async_work_group_copy_3D3D.
  };

  /// @see ExtInstSetHandler::create
  virtual llvm::Error create(const OpExtInst &opc) override;

 private:
  /// @brief Create a Codeplay.GroupAsyncCopies extended instruction
  /// transformation to LLVM IR.
  ///
  /// @tparam inst The Codeplay.GroupAsyncCopies extended instruction to
  /// create.
  /// @param opc The spirv_ll::OpCode object to translate.
  ///
  /// @return Returns an `llvm::Error` object representing either success, or
  /// an error value.
  template <Instruction inst>
  llvm::Error create(const OpExtInst &opc);
};

}  // namespace spirv_ll

#endif  // SPIRV_LL_SPV_BUILDER_GROUP_ASYNC_COPIES_H_INCLUDED
