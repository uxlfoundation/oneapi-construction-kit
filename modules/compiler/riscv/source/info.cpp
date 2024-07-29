// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
  kernel_debug = true;
}

std::unique_ptr<compiler::Target> RiscvInfo::createTarget(
    compiler::Context *context, compiler::NotifyCallbackFn callback) const {
  if (!context) {
    return nullptr;
  }
  return std::make_unique<riscv::RiscvTarget>(this, riscv_hal_device_info,
                                              context, callback);
}

}  // namespace riscv
