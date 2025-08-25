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
