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

#include <base/pass_pipelines.h>
#include <compiler/utils/cl_builtin_info.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/Target/TargetMachine.h>
#include <refsi_m1/module.h>
#include <refsi_m1/refsi_mux_builtin_info.h>
#include <refsi_m1/refsi_pass_machinery.h>
#include <refsi_m1/target.h>
#include <vecz/pass.h>

namespace refsi_m1 {
RefSiM1Module::RefSiM1Module(RefSiM1Target &target,
                             compiler::BaseContext &context,
                             uint32_t &num_errors, std::string &log)
    : riscv::RiscvModule(target, context, num_errors, log) {}

std::unique_ptr<compiler::utils::PassMachinery>
RefSiM1Module::createPassMachinery(llvm::LLVMContext &C) {
  auto *TM = getTargetMachine();
  auto *Builtins = getTarget().getBuiltins();
  const auto &BaseContext = getTarget().getContext();

  compiler::utils::DeviceInfo Info = compiler::initDeviceInfoFromMux(
      getTarget().getCompilerInfo()->device_info);

  auto Callback = [Builtins](const llvm::Module &) {
    return compiler::utils::BuiltinInfo(
        std::make_unique<RefSiM1BIMuxInfo>(),
        compiler::utils::createCLBuiltinInfo(Builtins));
  };
  return std::make_unique<RefSiM1PassMachinery>(
      getTarget(), C, TM, Info, Callback, BaseContext.isLLVMVerifyEachEnabled(),
      BaseContext.getLLVMDebugLoggingLevel(),
      BaseContext.isLLVMTimePassesEnabled());
}

llvm::ModulePassManager RefSiM1Module::getLateTargetPasses(
    compiler::utils::PassMachinery &pass_mach) {
  if (getOptions().llvm_stats) {
    llvm::EnableStatistics();
  }

  return static_cast<RefSiM1PassMachinery &>(pass_mach).getLateTargetPasses();
}

}  // namespace refsi_m1
