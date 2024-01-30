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

#ifndef SPIRV_LL_SPV_BUILDER_OPENCL_H_INCLUDED
#define SPIRV_LL_SPV_BUILDER_OPENCL_H_INCLUDED

#include <spirv-ll/builder.h>

namespace spirv_ll {

class OpenCLBuilder : public spirv_ll::ExtInstSetHandler {
 public:
  /// @brief Constructor.
  ///
  /// @param builder `Builder` object that will own this object.
  /// @param module The module being translated.
  OpenCLBuilder(Builder &builder, Module &module)
      : ExtInstSetHandler(builder, module) {}

  /// @see ExtInstSetHandler::create
  virtual llvm::Error create(const OpExtInst &opc) override;

 private:
  /// @brief Create an OpenCL extended instruction transformation to LLVM IR.
  ///
  /// @tparam T The OpenCL extended instruction class template to create.
  /// @param opc The OpCode object to translate.
  ///
  /// @return Returns an `llvm::Error` object representing either success, or
  /// an error value.
  template <typename T>
  llvm::Error create(const OpExtInst &opc);

  /// @brief Create a vector OpenCL extended instruction transformation to LLVM
  /// IR.
  ///
  /// @tparam inst The OpenCL extended instruction to create.
  /// @param opc The OpCode object to translate.
  ///
  /// @return Returns an `llvm::Error` object representing either success, or
  /// an error value.
  template <OpenCLLIB::Entrypoints inst>
  llvm::Error createVec(const OpExtInst &opc);
};

}  // namespace spirv_ll

#endif  // SPIRV_LL_SPV_BUILDER_OPENCL_H_INCLUDED
