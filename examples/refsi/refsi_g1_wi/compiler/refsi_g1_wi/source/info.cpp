// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <base/context.h>
#include <refsi_g1_wi/info.h>
#include <refsi_g1_wi/target.h>

namespace riscv {
struct hal_device_info_riscv_t;
}

namespace refsi_g1_wi {

RefSiG1Info::RefSiG1Info(mux_device_info_t mux_device_info,
                         const riscv::hal_device_info_riscv_t *hal_device_info)
    : RiscvInfo(mux_device_info, hal_device_info) {}

std::unique_ptr<compiler::Target> RefSiG1Info::createTarget(
    compiler::Context *context, compiler::NotifyCallbackFn callback) const {
  if (!context) {
    return nullptr;
  }
  return multi_llvm::make_unique<RefSiG1Target>(this, riscv_hal_device_info,
                                                context, callback);
}

}  // namespace refsi_g1_wi
