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

#ifndef {{cookiecutter.target_name_capitals}}_INFO_H
#define {{cookiecutter.target_name_capitals}}_INFO_H
#include <compiler/info.h>
#include <{{cookiecutter.target_name}}/device_info_get.h>

#include <mutex>
#include <vector>

namespace {{cookiecutter.target_name}} {
struct {{cookiecutter.target_name.capitalize()}}Info : compiler::Info {
  {{cookiecutter.target_name.capitalize()}}Info(mux_device_info_t mux_device_info);
 
  std::unique_ptr<compiler::Target> createTarget(
      compiler::Context *context, compiler::NotifyCallbackFn callback) const override;

   static void get(compiler::AddCompilerFn add_compiler) {
    static std::vector<{{cookiecutter.target_name.capitalize()}}Info> infos;
    static std::once_flag compilers_initialized;
    std::call_once(compilers_initialized, [] {
      for (auto &device_info : {{cookiecutter.target_name}}::GetDeviceInfosArray()) {
        infos.push_back({{cookiecutter.target_name.capitalize()}}Info(&device_info));
      }
    });
    for (const auto &info : infos) {
      add_compiler(&info);
    }
  }
};
}  // namespace {{cookiecutter.target_name}}

#endif  // {{cookiecutter.target_name_capitals}}_INFO_H
