
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

#include <spirv-ll/builder_debug_info.h>

namespace spirv_ll {

llvm::Error DebugInfoBuilder::create(OpExtInst const &) {
  // We currently let all of these instructions through without question. From
  // the OpenCL.DebugInfo.100 instruction set:
  // 2.1 Removing Instructions
  //   All instructions in this extended set have no semantic impact and can be
  //   safely removed. This is easily done if all debug instructions are removed
  //   together, at once. However, when removing a subset, for example, inlining
  //   a function, there may be dangling references to <id> that have been
  //   removed. These can be replaced with the Result <id> of the DebugInfoNone
  //   instruction.

  // Note that this does still assume that none of these instructions are
  // expected to produce LLVM values for anything other than non-semantic
  // instructions in these same extended instruction sets (mixing and matching
  // DebugInfo OpenCL.DebugInfo.100 is fine). However, it's an unlikely
  // scenario that anything produced by instructions in these sets is used by
  // another instruction set we support: these instructions are all debug info,
  // and all instructions in this set return 'OpTypeVoid' so can't really be
  // used/referenced by most ops in a meaningful way anyway.
  return llvm::Error::success();
}

}  // namespace spirv_ll
