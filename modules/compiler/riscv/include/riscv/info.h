// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef RISCV_INFO_H
#define RISCV_INFO_H

#include <compiler/info.h>
#include <compiler/module.h>
#include <multi_llvm/multi_llvm.h>
#include <riscv/target.h>

#include <mutex>
#include <vector>

namespace riscv {
class RiscvModule;
class RiscvTarget;
struct hal_device_info_riscv_t;

struct RiscvInfo : compiler::Info {
  RiscvInfo(mux_device_info_t mux_device_info,
            const hal_device_info_riscv_t *hal_device_info);
  virtual ~RiscvInfo() = default;

  std::unique_ptr<compiler::Target> createTarget(
      compiler::Context *context,
      compiler::NotifyCallbackFn callback) const override;

 protected:
  const riscv::hal_device_info_riscv_t *riscv_hal_device_info;
};
}  // namespace riscv

#endif  // RISCV_INFO_H
