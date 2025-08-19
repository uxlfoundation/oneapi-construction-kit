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

#include "riscv/buffer.h"

#include "riscv/device.h"
#include "riscv/memory.h"
#include "riscv/riscv.h"

mux_result_t riscvCreateBuffer(mux_device_t device, size_t size,
                               mux_allocator_info_t allocator_info,
                               mux_buffer_t *out_buffer) {
  auto buffer = riscv::buffer_s::create<riscv::buffer_s>(
      static_cast<riscv::device_s *>(device), size, allocator_info);
  if (!buffer) {
    return buffer.error();
  }
  *out_buffer = buffer.value();
  return mux_success;
}

void riscvDestroyBuffer(mux_device_t device, mux_buffer_t buffer,
                        mux_allocator_info_t allocator_info) {
  riscv::buffer_s::destroy(static_cast<riscv::device_s *>(device),
                           static_cast<riscv::buffer_s *>(buffer),
                           allocator_info);
}

mux_result_t riscvBindBufferMemory(mux_device_t device, mux_memory_t memory,
                                   mux_buffer_t buffer, uint64_t offset) {
  return static_cast<riscv::buffer_s *>(buffer)->bind(
      static_cast<riscv::device_s *>(device),
      static_cast<riscv::memory_s *>(memory), offset);
}
