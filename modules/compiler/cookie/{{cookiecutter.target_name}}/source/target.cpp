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

#include <hal.h>

#include <llvm/Target/TargetMachine.h>
#include <multi_llvm/multi_llvm.h>
#include <{{cookiecutter.target_name}}/device_info.h>
#include <{{cookiecutter.target_name}}/module.h>
#include <{{cookiecutter.target_name}}/target.h>
#include "{{cookiecutter.target_name}}/module.h"

#if LLVM_VERSION_GREATER_EQUAL(18, 0)
#include <llvm/TargetParser/Host.h>
#else
#include <llvm/Support/Host.h>
#endif

namespace {{cookiecutter.target_name}} {
{{cookiecutter.target_name.capitalize()}}Target::{{cookiecutter.target_name.capitalize()}}Target(const compiler::Info *compiler_info,
                             compiler::Context *context,
                             compiler::NotifyCallbackFn callback)
    : BaseAOTTarget(compiler_info, context, callback) {
  env_debug_prefix = "CA_{{cookiecutter.target_name_capitals}}";
  llvm_cpu = {{cookiecutter.llvm_cpu}};
  llvm_triple = {{cookiecutter.llvm_triple}};
  llvm_features = {{cookiecutter.llvm_features}};

  auto *{{cookiecutter.target_name}}_device_info =
      static_cast<{{cookiecutter.target_name}}::device_info_s *>(compiler_info->device_info);
  hal_device_info = {{cookiecutter.target_name}}_device_info->hal_device_info;
}

{{cookiecutter.target_name.capitalize()}}Target::~{{cookiecutter.target_name.capitalize()}}Target() {}

compiler::Result {{cookiecutter.target_name.capitalize()}}Target::initWithBuiltins(
    std::unique_ptr<llvm::Module> builtins_module) {
  builtins = std::move(builtins_module);

  return compiler::Result::SUCCESS;
}

std::unique_ptr<compiler::Module> {{cookiecutter.target_name.capitalize()}}Target::createModule(uint32_t &num_errors,
                                                       std::string &log) {
  return std::make_unique<{{cookiecutter.target_name.capitalize()}}Module>(
      *this, static_cast<compiler::BaseContext &>(context), num_errors, log);
}
}  // namespace {{cookiecutter.target_name}}
