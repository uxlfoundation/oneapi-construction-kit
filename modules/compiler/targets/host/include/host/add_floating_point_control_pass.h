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
/// Add floating-point control pass.

#ifndef HOST_ADD_FLOATING_POINT_CONTROL_PASS_H_INCLUDED
#define HOST_ADD_FLOATING_POINT_CONTROL_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace host {

/// @brief A pass that will replace any local work item functions.
///
/// Runs over all kernels with "kernel" metadata.
class AddFloatingPointControlPass final
    : public llvm::PassInfoMixin<AddFloatingPointControlPass> {
 public:
  AddFloatingPointControlPass(bool FTZ) : SetFTZ(FTZ) {}

  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);

 private:
  bool SetFTZ;
};
}  // namespace host

#endif  // HOST_ADD_FLOATING_POINT_CONTROL_PASS_H_INCLUDED
