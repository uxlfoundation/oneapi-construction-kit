// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
{% if cookiecutter.copyright_name != "" -%}
/// Additional changes Copyright (C) {{cookiecutter.copyright_name}}. All Rights
/// Reserved.
{% endif -%}

#ifndef {{cookiecutter.target_name_capitals}}_PASS_MACHINERY_H_INCLUDED
#define {{cookiecutter.target_name_capitals}}_PASS_MACHINERY_H_INCLUDED

#include <base/base_module_pass_machinery.h>
#include <{{cookiecutter.target_name}}/target.h>

namespace {{cookiecutter.target_name}} {

/// @brief Version of PassMachinery used in {{cookiecutter.target_name}}
/// @note This can be used to contain things that can be accessed
/// by various passes as we run through the passes.
class {{cookiecutter.target_name.capitalize()}}PassMachinery : public compiler::BaseModulePassMachinery {
 public:
  {{cookiecutter.target_name.capitalize()}}PassMachinery(
      const {{cookiecutter.target_name.capitalize()}}Target &target, llvm::LLVMContext &Ctx,
      llvm::TargetMachine *TM, const compiler::utils::DeviceInfo &Info,
      compiler::utils::BuiltinInfoAnalysis::CallbackFn BICallback,
      bool verifyEach, compiler::utils::DebugLogging debugLogLevel,
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

 private:
   const {{cookiecutter.target_name.capitalize()}}Target &target;
};

}  // namespace {{cookiecutter.target_name}}

#endif  // {{cookiecutter.target_name_capitals}}_PASS_MACHINERY_H_INCLUDED
