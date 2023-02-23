// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef RISCV_GET_INFO_H
#define RISCV_GET_INFO_H

#include <compiler/info.h>
#include <riscv/device_info_get.h>
#include <riscv/info.h>

#include <vector>

namespace riscv {
static void getInfos(compiler::AddCompilerFn add_compiler) {
  static std::vector<RiscvInfo> infos;
  static std::once_flag compilers_initialized;
  std::call_once(compilers_initialized, [] {
    for (auto &device_info : riscv::GetDeviceInfosArray()) {
      auto *riscv_hal_device_info =
          static_cast<const ::riscv::hal_device_info_riscv_t *>(
              device_info.hal_device_info);
      infos.push_back(RiscvInfo(&device_info, riscv_hal_device_info));
    }
  });
  for (const auto &info : infos) {
    add_compiler(&info);
  }
}
}  // namespace riscv
#endif
