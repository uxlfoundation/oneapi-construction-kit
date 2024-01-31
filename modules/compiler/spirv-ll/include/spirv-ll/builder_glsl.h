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

#ifndef SPIRV_LL_SPV_BUILDER_GLSL_H_INCLUDED
#define SPIRV_LL_SPV_BUILDER_GLSL_H_INCLUDED

#include <spirv-ll/builder.h>
#include <spirv/unified1/GLSL.std.450.h>

namespace spirv_ll {

class GLSLBuilder : public ExtInstSetHandler {
 public:
  /// @brief Constructor.
  ///
  /// @param builder `Builder` object that will own this object.
  /// @param module The module being translated.
  GLSLBuilder(Builder &builder, Module &module)
      : ExtInstSetHandler(builder, module) {}

  /// @see ExtendedInstrSet::create
  virtual llvm::Error create(const OpExtInst &opc) override;

 private:
  /// @brief Create a GLSL extended instruction transformation to LLVM IR.
  ///
  /// @tparam inst The GLSL extended instruction to create.
  /// @param opc The OpCode object to translate.
  ///
  /// @return Returns an `llvm::Error` object representing either success, or
  /// an error value.
  template <enum GLSLstd450 inst>
  llvm::Error create(const OpExtInst &opc);
};

}  // namespace spirv_ll

#endif  // SPIRV_LL_SPV_BUILDER_GLSL_H_INCLUDED
