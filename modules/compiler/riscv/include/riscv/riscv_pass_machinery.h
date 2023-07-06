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

#ifndef RISCV_PASS_MACHINERY_H_INCLUDED
#define RISCV_PASS_MACHINERY_H_INCLUDED

#include <base/base_pass_machinery.h>
#include <riscv/target.h>
#include <vecz/pass.h>

#include <optional>

namespace riscv {

/// @brief Version of PassMachinery used in risc-v
/// @note This can be used to contain things that can be accessed
/// by various passes as we run through the passes.
class RiscvPassMachinery : public compiler::BaseModulePassMachinery {
 public:
  RiscvPassMachinery(
      const riscv::RiscvTarget &target, llvm::LLVMContext &Ctx,
      llvm::TargetMachine *TM, const compiler::utils::DeviceInfo &Info,
      compiler::utils::BuiltinInfoAnalysis::CallbackFn BICallback,
      bool verifyEach, compiler::utils::DebugLogging debugLogging,
      bool timePasses);

  void addClassToPassNames() override;

  void registerPasses() override;

  void registerPassCallbacks() override;

  void printPassNames(llvm::raw_ostream &OS) override;

  bool handlePipelineElement(llvm::StringRef,
                             llvm::ModulePassManager &AM) override;

  /// @brief Returns an optimization pass pipeline to run over all kernels in a
  /// module. @see BaseModule::getLateTargetPasses.
  ///
  /// @return Result ModulePassManager containing passes
  llvm::ModulePassManager getLateTargetPasses();

  struct OptimizationOptions {
    llvm::SmallVector<vecz::VeczPassOptions> vecz_pass_opts;
    bool force_no_tail = false;
    bool early_link_builtins = false;
  };

  static OptimizationOptions processOptimizationOptions(
      std::optional<std::string> env_debug_prefix,
      std::optional<compiler::VectorizationMode> vecz_mode);

 protected:
  const riscv::RiscvTarget &target;
};

}  // namespace riscv

#endif  // RISCV_PASS_MACHINERY_H_INCLUDED
