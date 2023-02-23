// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <base/context.h>
#include <compiler/module.h>
#include <hal_riscv.h>
#include <llvm/Target/CodeGenCWrappers.h>
#include <llvm/Target/TargetMachine.h>
#include <multi_llvm/llvm_version.h>
#include <multi_llvm/multi_llvm.h>
#include <riscv/info.h>
#include <riscv/module.h>
#include <riscv/target.h>

namespace riscv {

RiscvInfo::RiscvInfo(mux_device_info_t mux_device_info,
                     const hal_device_info_riscv_t *hal_device_info)
    : compiler::Info() {
  device_info = mux_device_info;

  riscv_hal_device_info = hal_device_info;

  scalable_vector_support =
      riscv_hal_device_info->extensions & ::riscv::rv_extension_V;

  vectorizable = false;
  dma_optimizable = true;
  supports_deferred_compilation = false;
  kernel_debug = true;
}

std::unique_ptr<compiler::Target> RiscvInfo::createTarget(
    compiler::Context *context, compiler::NotifyCallbackFn callback) const {
  if (!context) {
    return nullptr;
  }
  return multi_llvm::make_unique<riscv::RiscvTarget>(
      this, riscv_hal_device_info, context, callback);
}

}  // namespace riscv
