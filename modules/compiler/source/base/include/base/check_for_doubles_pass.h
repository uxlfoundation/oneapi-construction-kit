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
/// @brief Class CheckForDoublesPass

#ifndef BASE_CHECK_FOR_DOUBLES_PASS_H_INCLUDED
#define BASE_CHECK_FOR_DOUBLES_PASS_H_INCLUDED

#include <llvm/IR/DiagnosticInfo.h>
#include <llvm/IR/PassManager.h>

namespace llvm {
class DiagnosticPrinter;
}

namespace compiler {

struct DiagnosticInfoDoubleNoDouble : public llvm::DiagnosticInfo {
  static int DK_DoubleNoDouble;

  DiagnosticInfoDoubleNoDouble()
      : llvm::DiagnosticInfo(DK_DoubleNoDouble, llvm::DS_Error) {}

  llvm::StringRef formatMessage() const;

  void print(llvm::DiagnosticPrinter &) const override;

  static bool classof(const llvm::DiagnosticInfo *DI) {
    return DI->getKind() == DK_DoubleNoDouble;
  }
};

/// @addtogroup cl_compiler
/// @{

/// @brief Pass to check for FP doubles.
///
/// This pass is used to check for the presence of floating-point doubles.
/// Doubles are optional in OpenCL (cl_khr_fp64), and if we don't have them, we
/// need to check that we aren't using them.

/// All basic blocks containing any instruction with 'double'-typed operands or
/// return type are raised as external functions are raised as
/// DiagnosticInfoDoubleNoDouble diagnostics with error-level severity. The
/// currently installed diagnostic handler is responsible for handling them. It
/// may abort, it may log an error and continue, or it may ignore them
/// completely; there are no requirements imposed by ComputeMux.
///
/// Note that the compilation pipeline will continue after this pass unless the
/// diagnostic stops it.
struct CheckForDoublesPass final
    : public llvm::PassInfoMixin<CheckForDoublesPass> {
  /// @brief The entry point to the pass.
  ///
  /// @param F The Function provided to the pass.
  /// @param AM Manager providing analyses.
  ///
  /// @return Return whether or not the pass changed anything (always false for
  /// this pass).
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);

  // This pass is not an optimization
  static bool isRequired() { return true; }
};

/// @}
}  // namespace compiler

#endif  // BASE_CHECK_FOR_DOUBLES_PASS_H_INCLUDED
