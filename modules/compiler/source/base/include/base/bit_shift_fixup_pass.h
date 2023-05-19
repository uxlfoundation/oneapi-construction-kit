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
/// @brief Class BitShiftFixupPass interface.
///
/// Shift instructions need to be updated: SPIR defines them to have defined
/// results with oversized shift amounts but LLVM IR does not. 'modulo N'
/// needs to be performed to the shift amount prior to the shift, where N is
/// the bit width of the value to shift.

#ifndef BASE_BIT_SHIFT_FIXUP_PASS_H_INCLUDED
#define BASE_BIT_SHIFT_FIXUP_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
/// @addtogroup cl_compiler
/// @{

/// @brief Update shift instructions to add a modulo when required.
struct BitShiftFixupPass final : public llvm::PassInfoMixin<BitShiftFixupPass> {
  BitShiftFixupPass() = default;

  /// @brief The entry point to the pass.
  ///
  /// @param[in,out] F The function provided to the pass.
  /// @param[in,out] FAM The analysis manager provided to the pass.
  ///
  /// @return Return analyses preserved by the pass
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM);

  // This pass is not an optimization
  static bool isRequired() { return true; }
};

/// @}
}  // namespace compiler

#endif  // BASE_BIT_SHIFT_FIXUP_PASS_H_INCLUDED
