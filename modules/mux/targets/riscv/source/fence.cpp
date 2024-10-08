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
#include <riscv/fence.h>

#include "mux/mux.h"
#include "riscv/riscv.h"

mux_result_t riscvCreateFence(mux_device_t device,
                              mux_allocator_info_t allocator_info,
                              mux_fence_t *out_fence) {
  const mux::allocator allocator(allocator_info);
  auto fence = riscv::fence_s::create<riscv::fence_s>(device, allocator);
  if (!fence) {
    return mux_error_out_of_memory;
  }
  *out_fence = *fence;
  return mux_success;
}

void riscvDestroyFence(mux_device_t device, mux_fence_t fence,
                       mux_allocator_info_t allocator_info) {
  riscv::fence_s::destroy(device, static_cast<riscv::fence_s *>(fence),
                          mux::allocator(allocator_info));
}

mux_result_t riscvResetFence(mux_fence_t fence) {
  static_cast<riscv::fence_s *>(fence)->reset();
  return mux_success;
}
