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
/// RISC-V IR-to-Builtin replacement pass.

#ifndef RISCV_IR_TO_BUILTINS_PASS_H_INCLUDED
#define RISCV_IR_TO_BUILTINS_PASS_H_INCLUDED

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/PassManager.h>

namespace riscv {
/// @brief Pass mapping IR instructions to builtins.
///
/// Maps IR instructions to OpenCL builtins
/// Currently only supports frem and does not consider constant expressions
/// This avoids link errors for fmodf and fmod. Preferred solution is to
/// create a library we can link with.
struct IRToBuiltinReplacementPass final
    : public llvm::PassInfoMixin<IRToBuiltinReplacementPass> {
  bool replaceInstruction(llvm::Module &module, unsigned opcode,
                          llvm::StringRef name);

  llvm::PreservedAnalyses run(llvm::Module &module,
                              llvm::ModuleAnalysisManager &AM);

  IRToBuiltinReplacementPass() {}
};

}  // namespace riscv

#endif  // RISCV_IR_TO_BUILTINS_PASS_H_INCLUDED
