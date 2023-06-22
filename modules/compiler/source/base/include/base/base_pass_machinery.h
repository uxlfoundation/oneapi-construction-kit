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
/// @brief Base module pass machinery to be used for BaseModule's PassManager
/// state

#ifndef BASE_PASS_MACHINERY_H_INCLUDED
#define BASE_PASS_MACHINERY_H_INCLUDED

#include <compiler/module.h>
#include <compiler/utils/builtin_info.h>
#include <compiler/utils/device_info.h>
#include <compiler/utils/pass_machinery.h>
#include <mux/mux.h>

namespace llvm {
class Module;
class ModulePass;
}  // namespace llvm

namespace compiler {
/// @addtogroup compiler
/// @{

class BaseModulePassMachinery : public compiler::utils::PassMachinery {
 public:
  BaseModulePassMachinery(
      llvm::LLVMContext &Ctx, llvm::TargetMachine *TM,
      multi_llvm::Optional<compiler::utils::DeviceInfo> Info,
      compiler::utils::BuiltinInfoAnalysis::CallbackFn BICallback,
      bool verifyEach, compiler::utils::DebugLogging debugLogging,
      bool timePasses)
      : compiler::utils::PassMachinery(Ctx, TM, verifyEach, debugLogging),
        TimePasses(timePasses),
        Info(Info),
        BICallback(BICallback) {}

  virtual void registerPasses() override;
  virtual void addClassToPassNames() override;
  virtual void registerPassCallbacks() override;
  virtual void printPassNames(llvm::raw_ostream &OS) override;

  /// @brief Sets up compiler options on this machinery.
  void setCompilerOptions(const compiler::Options &options);

  /// @brief Provides derived classes with a way to hook in custom pipeline
  /// elements.
  /// @return true if the pipeline element was parsed and handled, false
  /// otherwise.
  virtual bool handlePipelineElement(llvm::StringRef,
                                     llvm::ModulePassManager &) {
    return false;
  }

 protected:
  compiler::Options options;

 private:
  llvm::TimePassesHandler TimePasses;

  /// @brief Device-specific information about the ComputeMux target being
  /// compiled for.
  multi_llvm::Optional<compiler::utils::DeviceInfo> Info;

  /// @brief An optional callback function that provides target-specific
  /// BuiltinInfo information to supply to the BuiltinInfoAnalysis analysis
  /// pass.
  std::function<compiler::utils::BuiltinInfo(const llvm::Module &)> BICallback;
};

/// @brief A function which transfers 'mux' device properties to 'compiler'
/// ones.
///
/// FIXME: Ideally we wouldn't have any mux in the compiler library. See
/// CA-4236.
compiler::utils::DeviceInfo initDeviceInfoFromMux(
    mux_device_info_t device_info);

/// @}
}  // namespace compiler
#endif  // BASE_PASS_MACHINERY_H_INCLUDED
