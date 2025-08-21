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

#include <host/device.h>
#include <host/host.h>
#include <host/memory.h>
#include <mux/utils/allocator.h>

#include <cstring>

namespace host {
memory_s::memory_s(uint64_t size, uint32_t properties, void *data, bool useHost)

    : data(data), useHost(useHost) {
  this->size = size;
  this->properties = properties;
  this->handle = reinterpret_cast<uintptr_t>(data);
}
} // namespace host

mux_result_t hostAllocateMemory(mux_device_t device, size_t size, uint32_t heap,
                                uint32_t memory_properties,
                                mux_allocation_type_e allocation_type,
                                uint32_t alignment,
                                mux_allocator_info_t allocator_info,
                                mux_memory_t *out_memory) {
  (void)device;
  (void)allocation_type;

  mux::allocator allocator(allocator_info);

  // Ensure the specified heap is valid, as heaps are target specific the check
  // must be performed by the target. Note that this is a proof of concept
  // implementation since host only has a single memory heap to allocate from,
  // in future this should be removed when an example of a mux target which
  // requires multiple memory heaps is provided.
  switch (heap) {
  case host::memory_s::HEAP_ALL:
  case host::memory_s::HEAP_BUFFER:
  case host::memory_s::HEAP_IMAGE:
    break;
  default:                          // GCOVR_EXCL_LINE ...
    return mux_error_invalid_value; // GCOVR_EXCL_LINE ...
                                    // non-deterministically executed
  }

  // Align all allocations to at least 128 bytes to match the size of the
  // largest 16-wide OpenCL-C vector types.
  const size_t host_align = std::max(128u, alignment);
  void *host_pointer = allocator.alloc(size, host_align);
  if (nullptr == host_pointer) {
    return mux_error_out_of_memory;
  }

  auto memory = allocator.create<host::memory_s>(size, memory_properties,
                                                 host_pointer, false);
  if (nullptr == memory) {
    allocator.free(host_pointer);
    return mux_error_out_of_memory;
  }

  *out_memory = memory;

  return mux_success;
}

mux_result_t hostCreateMemoryFromHost(mux_device_t device, size_t size,
                                      void *host_pointer,
                                      mux_allocator_info_t allocator_info,
                                      mux_memory_t *out_memory) {
  (void)device;
  mux::allocator allocator(allocator_info);

  // Our host device has coherent memory with the host-side platform
  const uint32_t memory_properties =
      mux_memory_property_host_visible | mux_memory_property_host_coherent;
  auto memory = allocator.create<host::memory_s>(size, memory_properties,
                                                 host_pointer, true);

  if (nullptr == memory) {
    return mux_error_out_of_memory;
  }

  *out_memory = memory;

  return mux_success;
}

void hostFreeMemory(mux_device_t device, mux_memory_t memory,
                    mux_allocator_info_t allocator_info) {
  (void)device;
  mux::allocator allocator(allocator_info);

  auto hostMemory = static_cast<host::memory_s *>(memory);

  if (!hostMemory->useHost) {
    allocator.free(hostMemory->data);
  }

  allocator.destroy(hostMemory);
}

mux_result_t hostMapMemory(mux_device_t device, mux_memory_t memory,
                           uint64_t offset, uint64_t size, void **out_data) {
  (void)device;

  auto hostMemory = static_cast<host::memory_s *>(memory);

  // NOTE: On host we can't map a range of virtual memory because the entire
  // memory block is already addressable due to using unified memory.
  (void)size;
  void *addressMap = hostMemory->data;
  *out_data = static_cast<unsigned char *>(addressMap) + offset;

  return mux_success;
}

mux_result_t hostFlushMappedMemoryToDevice(mux_device_t device,
                                           mux_memory_t memory, uint64_t offset,
                                           uint64_t size) {
  (void)device;
  (void)memory;
  (void)offset;
  (void)size;

  // NOTE: On host flushing is a noop because we take advantage of unified
  // memory.

  return mux_success;
}

mux_result_t hostFlushMappedMemoryFromDevice(mux_device_t device,
                                             mux_memory_t memory,
                                             uint64_t offset, uint64_t size) {
  (void)device;
  (void)memory;
  (void)offset;
  (void)size;

  // NOTE: On host flushing is a noop because we take advantage of unified
  // memory.

  return mux_success;
}

mux_result_t hostUnmapMemory(mux_device_t device, mux_memory_t memory) {
  (void)device;
  (void)memory;

  // NOTE: On host unmap is a noop because we take advantage of unified memory.

  return mux_success;
}
