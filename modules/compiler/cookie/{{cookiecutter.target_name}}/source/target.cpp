/// Copyright (C) Codeplay Software Limited. All Rights Reserved.
{% if cookiecutter.copyright_name != "" -%}
/// Additional changes Copyright (C) {{cookiecutter.copyright_name}}. All Rights
/// Reserved.
{% endif -%}

#include <hal.h>

#include <llvm/Support/Host.h>
#include <llvm/Target/TargetMachine.h>
#include <multi_llvm/multi_llvm.h>
#include <{{cookiecutter.target_name}}/device_info.h>
#include <{{cookiecutter.target_name}}/module.h>
#include <{{cookiecutter.target_name}}/target.h>
#include "{{cookiecutter.target_name}}/module.h"

namespace {{cookiecutter.target_name}} {
{{cookiecutter.target_name.capitalize()}}Target::{{cookiecutter.target_name.capitalize()}}Target(const compiler::Info *compiler_info,
                             compiler::Context *context,
                             compiler::NotifyCallbackFn callback)
    : BaseTarget(compiler_info, context, callback) {
  env_debug_prefix = "CA_{{cookiecutter.target_name_capitals}}";
  available_snapshots = {
      compiler::BaseModule::getTargetSnapshotName("{{cookiecutter.target_name}}",
                                                  {{cookiecutter.target_name_capitals}}_SNAPSHOT_INPUT),
      compiler::BaseModule::getTargetSnapshotName("{{cookiecutter.target_name}}",
                                                  {{cookiecutter.target_name_capitals}}_SNAPSHOT_VECTORIZED),
      compiler::BaseModule::getTargetSnapshotName("{{cookiecutter.target_name}}",
                                                  {{cookiecutter.target_name_capitals}}_SNAPSHOT_BARRIER),
      compiler::BaseModule::getTargetSnapshotName("{{cookiecutter.target_name}}",
                                                  {{cookiecutter.target_name_capitals}}_SNAPSHOT_SCHEDULED),
      compiler::BaseModule::getTargetSnapshotName("{{cookiecutter.target_name}}",
                                                  {{cookiecutter.target_name_capitals}}_SNAPSHOT_BACKEND),
  };
  supported_target_snapshots.insert(supported_target_snapshots.begin(),
                                    available_snapshots.begin(),
                                    available_snapshots.end());
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
  return multi_llvm::make_unique<{{cookiecutter.target_name.capitalize()}}Module>(
      *this, static_cast<compiler::BaseContext &>(context), num_errors, log);
}
}  // namespace {{cookiecutter.target_name}}
