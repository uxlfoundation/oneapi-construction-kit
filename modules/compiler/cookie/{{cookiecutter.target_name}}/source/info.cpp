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
{% if cookiecutter.copyright_name != "" -%}
/// Additional changes Copyright (C) {{cookiecutter.copyright_name}}. All Rights
/// Reserved.
{% endif -%}

#include <compiler/module.h>
#include <hal_riscv.h>
#include <llvm/Target/CodeGenCWrappers.h>
#include <multi_llvm/llvm_version.h>
#include <multi_llvm/multi_llvm.h>
#include <{{cookiecutter.target_name}}/device_info.h>
#include <{{cookiecutter.target_name}}/info.h>
#include <{{cookiecutter.target_name}}/target.h>
{% if "refsi_wrapper_pass"  in cookiecutter.feature.split(";") -%}
#include <{{cookiecutter.target_name}}/refsi_wrapper_pass.h>
{% endif -%}

namespace {{cookiecutter.target_name}} {

{{cookiecutter.target_name.capitalize()}}Info::{{cookiecutter.target_name.capitalize()}}Info(mux_device_info_t mux_device_info) : compiler::Info() {
  device_info = mux_device_info;
  vectorizable = true;
  dma_optimizable = true;
  scalable_vector_support = {{cookiecutter.scalable_vector}};
  kernel_debug = true;

  static std::once_flag llvm_initialized;
  std::call_once(llvm_initialized, [&]() {
    // Init llvm targets.
    LLVMInitialize{{cookiecutter.llvm_name}}Target();
    LLVMInitialize{{cookiecutter.llvm_name}}TargetInfo();
    LLVMInitialize{{cookiecutter.llvm_name}}AsmPrinter();
    LLVMInitialize{{cookiecutter.llvm_name}}TargetMC();
    LLVMInitialize{{cookiecutter.llvm_name}}AsmParser();
  });
}

std::unique_ptr<compiler::Target> {{cookiecutter.target_name.capitalize()}}Info::createTarget(
    compiler::Context *context, compiler::NotifyCallbackFn callback) const {
  if (!context) {
    return nullptr;
  }

  return std::make_unique<{{cookiecutter.target_name}}::{{cookiecutter.target_name.capitalize()}}Target>(
      this, context, callback);
}

}  // namespace {{cookiecutter.target_name}}
