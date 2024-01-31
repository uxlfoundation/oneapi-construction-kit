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
/// Adds 3D work-group loops around a kernel

#ifndef RISCV_REFSI_WG_LOOP_PASS_H_INCLUDED
#define RISCV_REFSI_WG_LOOP_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace refsi_g1_wi {

class RefSiWGLoopPass final : public llvm::PassInfoMixin<RefSiWGLoopPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};

}  // namespace refsi_g1_wi

#endif  // RISCV_REFSI_WG_LOOP_PASS_H_INCLUDED
