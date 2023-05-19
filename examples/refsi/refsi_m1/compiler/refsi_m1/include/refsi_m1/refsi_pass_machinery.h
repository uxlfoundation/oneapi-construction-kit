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

#ifndef REFSI_M1_PASS_MACHINERY_H_INCLUDED
#define REFSI_M1_PASS_MACHINERY_H_INCLUDED

#include <riscv/riscv_pass_machinery.h>

namespace refsi_m1 {

/// @brief Version of PassMachinery used in RefSi M1 architecture
/// @note This can be used to contain things that can be accessed
/// by various passes as we run through the passes.
class RefSiM1PassMachinery : public riscv::RiscvPassMachinery {
 public:
  RefSiM1PassMachinery(
      llvm::LLVMContext &Ctx, llvm::TargetMachine *TM,
      const compiler::utils::DeviceInfo &Info,
      compiler::utils::BuiltinInfoAnalysis::CallbackFn BICallback,
      bool verifyEach, compiler::utils::DebugLogging debugLogging,
      bool timePasses);

  void addClassToPassNames() override;
  void registerPassCallbacks() override;
  void printPassNames(llvm::raw_ostream &OS) override;
};

}  // namespace refsi_m1

#endif  // REFSI_M1_PASS_MACHINERY_H_INCLUDED
