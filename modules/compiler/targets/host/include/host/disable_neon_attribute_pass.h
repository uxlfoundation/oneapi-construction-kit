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
/// Disable 'neon' attribute pass

#ifndef HOST_DISABLE_NEON_ATTRIBUTE_PASS_H_INCLUDED
#define HOST_DISABLE_NEON_ATTRIBUTE_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace host {

/// @brief A pass that will disables NEON support from all functions in the
/// module if the possibility of hitting a 64-bit ARM issue is encountered in
/// the IR.
/// The specific issue is that NEON vector conversions from i64 -> float do the
/// conversion in two stages: i64 -> double then double -> float. This loses
/// precision because of incorrect rounding in the intermediate value.
class DisableNeonAttributePass final
    : public llvm::PassInfoMixin<DisableNeonAttributePass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace host

#endif  // HOST_DISABLE_NEON_ATTRIBUTE_PASS_H_INCLUDED
