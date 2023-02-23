// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
  supports_deferred_compilation = false;
  scalable_vector_support = {{cookiecutter.scalable_vector}};
  kernel_debug = false;

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

  return multi_llvm::make_unique<{{cookiecutter.target_name}}::{{cookiecutter.target_name.capitalize()}}Target>(
      this, context, callback);
}

}  // namespace {{cookiecutter.target_name}}
