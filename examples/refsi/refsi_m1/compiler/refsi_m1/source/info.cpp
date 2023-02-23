// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <base/context.h>
#include <refsi_m1/info.h>
#include <refsi_m1/target.h>

namespace riscv {
struct hal_device_info_riscv_t;
}

namespace refsi_m1 {

RefSiM1Info::RefSiM1Info(mux_device_info_t mux_device_info,
                         const riscv::hal_device_info_riscv_t *hal_device_info)
    : RiscvInfo(mux_device_info, hal_device_info) {}

std::unique_ptr<compiler::Target> RefSiM1Info::createTarget(
    compiler::Context *context, compiler::NotifyCallbackFn callback) const {
  if (!context) {
    return nullptr;
  }
  return multi_llvm::make_unique<RefSiM1Target>(this, riscv_hal_device_info,
                                                context, callback);
}

}  // namespace refsi_m1
