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
