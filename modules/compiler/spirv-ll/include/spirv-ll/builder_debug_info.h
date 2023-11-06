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

#ifndef SPIRV_LL_SPV_BUILDER_DEBUG_INFO_H_INCLUDED
#define SPIRV_LL_SPV_BUILDER_DEBUG_INFO_H_INCLUDED

#include <spirv-ll/builder.h>

namespace spirv_ll {

/// @brief Combined builder for the DebugInfo and OpenCLDebugInfo100 extended
/// instruction sets.
class DebugInfoBuilder : public ExtInstSetHandler {
 public:
  /// @brief Constructor.
  ///
  /// @param[in] builder spirv_ll::Builder object that will own this object.
  /// @param[in] module The module being translated.
  DebugInfoBuilder(Builder &builder, Module &module)
      : ExtInstSetHandler(builder, module) {}

  /// @see ExtInstSetHandler::create
  virtual llvm::Error create(OpExtInst const &opc) override;
};

}  // namespace spirv_ll

#endif  // SPIRV_LL_SPV_BUILDER_DEBUG_INFO_H_INCLUDED
