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

#include <mux/utils/allocator.h>
#include <riscv/riscv.h>
#include <riscv/semaphore.h>

mux_result_t riscvCreateSemaphore(mux_device_t device,
                                  mux_allocator_info_t allocator_info,
                                  mux_semaphore_t *out_semaphore) {
  const mux::allocator allocator(allocator_info);
  auto semaphore =
      riscv::semaphore_s::create<riscv::semaphore_s>(device, allocator);
  if (!semaphore) {
    return semaphore.error();
  }
  *out_semaphore = semaphore.value();
  return mux_success;
}

void riscvDestroySemaphore(mux_device_t device, mux_semaphore_t semaphore,
                           mux_allocator_info_t allocator_info) {
  const mux::allocator allocator(allocator_info);
  riscv::semaphore_s::destroy(
      device, static_cast<riscv::semaphore_s *>(semaphore), allocator);
}

mux_result_t riscvResetSemaphore(mux_semaphore_t semaphore) {
  static_cast<riscv::semaphore_s *>(semaphore)->reset();
  return mux_success;
}
