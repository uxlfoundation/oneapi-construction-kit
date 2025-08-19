// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef REFSI_M1_INFO_H
#define REFSI_M1_INFO_H

#include <riscv/device_info_get.h>
#include <riscv/info.h>

#include <vector>

namespace refsi_m1 {

struct RefSiM1Info : riscv::RiscvInfo {
  RefSiM1Info(mux_device_info_t mux_device_info,
              const riscv::hal_device_info_riscv_t *hal_device_info);

  std::unique_ptr<compiler::Target> createTarget(
      compiler::Context *context,
      compiler::NotifyCallbackFn callback) const override;

  static void get(compiler::AddCompilerFn add_compiler) {
    static std::vector<RefSiM1Info> infos;
    static std::once_flag compilers_initialized;
    std::call_once(compilers_initialized, [] {
      for (auto &device_info : riscv::GetDeviceInfosArray()) {
        auto *riscv_hal_device_info =
            static_cast<const riscv::hal_device_info_riscv_t *>(
                device_info.hal_device_info);
        infos.push_back(RefSiM1Info(&device_info, riscv_hal_device_info));
      }
    });
    for (const auto &info : infos) {
      add_compiler(&info);
    }
  }
};
}  // namespace refsi_m1

#endif  // REFSI_M1_INFO_H
