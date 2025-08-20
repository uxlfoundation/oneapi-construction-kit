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
  return std::make_unique<RefSiG1Target>(this, riscv_hal_device_info, context,
                                         callback);
}

}  // namespace refsi_g1_wi
