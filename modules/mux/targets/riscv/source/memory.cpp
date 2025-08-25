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

#include <riscv/device.h>
#include <riscv/memory.h>
#include <riscv/riscv.h>

namespace riscv {
mux_result_t memory_s::flushToDevice(riscv::device_s *device, uint64_t offset,
                                     uint64_t size) {
  assert(device->hal_device);
  if (auto error =
          mux::hal::memory::flushToDevice(device->hal_device, offset, size)) {
    return error;
  }
  // TODO(CA-4163): Moved to mux::hal::memory once mux::hal::device exists.
  device->profiler.update_counters(*device->hal_device);
  return mux_success;
}

mux_result_t memory_s::flushFromDevice(riscv::device_s *device, uint64_t offset,
                                       uint64_t size) {
  if (auto error =
          mux::hal::memory::flushFromDevice(device->hal_device, offset, size)) {
    return error;
  }
  // TODO(CA-4163): Moved to mux::hal::memory once mux::hal::device exists.
  device->profiler.update_counters(*device->hal_device);
  return mux_success;
}
}  // namespace riscv

mux_result_t riscvAllocateMemory(mux_device_t device, size_t size,
                                 uint32_t heap, uint32_t memory_properties,
                                 mux_allocation_type_e allocation_type,
                                 uint32_t alignment,
                                 mux_allocator_info_t allocator_info,
                                 mux_memory_t *out_memory) {
  // TODO(CA-4163): Cast to mux::hal::device and pass in directly.
  hal::hal_device_t *hal_device =
      static_cast<riscv::device_s *>(device)->hal_device;
  assert(hal_device);
  auto memory = riscv::memory_s::create<riscv::memory_s>(
      hal_device, size, heap, memory_properties, allocation_type, alignment,
      allocator_info);
  if (!memory) {
    return memory.error();
  }
  *out_memory = memory.value();
  return mux_success;
}

mux_result_t riscvCreateMemoryFromHost(mux_device_t device, size_t size,
                                       void *riscv_pointer,
                                       mux_allocator_info_t allocator_info,
                                       mux_memory_t *out_memory) {
  // TODO(CA-4163): Cast to mux::hal::device and pass in directly.
  hal::hal_device_t *hal_device =
      static_cast<riscv::device_s *>(device)->hal_device;
  assert(hal_device);
  auto memory = riscv::memory_s::create<riscv::memory_s>(
      hal_device, size, riscv_pointer, allocator_info);
  if (!memory) {
    return memory.error();
  }
  return mux_success;
}

void riscvFreeMemory(mux_device_t device, mux_memory_t memory,
                     mux_allocator_info_t allocator_info) {
  // TODO(CA-4163): Cast to mux::hal::device and pass in directly.
  hal::hal_device_t *hal_device =
      static_cast<riscv::device_s *>(device)->hal_device;
  assert(hal_device);
  riscv::memory_s::destroy(hal_device, static_cast<riscv::memory_s *>(memory),
                           allocator_info);
}

mux_result_t riscvMapMemory(mux_device_t device, mux_memory_t memory,
                            uint64_t offset, uint64_t size, void **out_data) {
  // TODO(CA-4163): Cast to mux::hal::device and pass in directly.
  hal::hal_device_t *hal_device =
      static_cast<riscv::device_s *>(device)->hal_device;
  assert(hal_device);
  auto data =
      static_cast<riscv::memory_s *>(memory)->map(hal_device, offset, size);
  if (!data) {
    return data.error();
  }
  *out_data = data.value();
  return mux_success;
}

mux_result_t riscvFlushMappedMemoryToDevice(mux_device_t device,
                                            mux_memory_t memory,
                                            uint64_t offset, uint64_t size) {
  return static_cast<riscv::memory_s *>(memory)->flushToDevice(
      static_cast<riscv::device_s *>(device), offset, size);
}

mux_result_t riscvFlushMappedMemoryFromDevice(mux_device_t device,
                                              mux_memory_t memory,
                                              uint64_t offset, uint64_t size) {
  return static_cast<riscv::memory_s *>(memory)->flushFromDevice(
      static_cast<riscv::device_s *>(device), offset, size);
}

mux_result_t riscvUnmapMemory(mux_device_t device, mux_memory_t memory) {
  hal::hal_device_t *hal_device =
      static_cast<riscv::device_s *>(device)->hal_device;
  assert(hal_device);
  return static_cast<riscv::memory_s *>(memory)->unmap(hal_device);
}
