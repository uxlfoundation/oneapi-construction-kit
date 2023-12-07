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

#ifndef COMPILER_UTILS_MANUAL_TYPE_LEGALIZATION_PASS_H_INCLUDED
#define COMPILER_UTILS_MANUAL_TYPE_LEGALIZATION_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// Manual type legalization pass.
///
/// On targets that do not natively support \c half, promote operations on \c
/// half to \c float instead.
///
/// When LLVM encounters floating point operations in a type it does not support
/// natively, it extends its operands to an extended precision floating point
/// type, performs the operation in that extended type, and rounds the result
/// back to the original type. However, when it extends its operands to an
/// extended precision floating point type, if an operand itself was a floating
/// point operation that was also so extended, its rounding and re-extension are
/// skipped. This causes issues for code that relies on exact rounding of
/// intermediate results, which we avoid by manually doing this promition
/// ourselves.
///
/// Simply performing operations in a wider floating point type and rounding
/// back to the narrow floating point type is not, in general, correct, due to
/// double rounding. For addition, subtraction, and multiplications, \c float
/// provides enough additional precision that double rounding is known not to be
/// an issue. For other operations, this pass may generate incorrect results,
/// but this should only happen in cases where letting the operation pass
/// through to LLVM would result in the same incorrect results.
struct ManualTypeLegalizationPass final
    : llvm::PassInfoMixin<ManualTypeLegalizationPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM);
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_MANUAL_TYPE_LEGALIZATION_PASS_H_INCLUDED
