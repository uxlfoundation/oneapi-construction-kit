// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
