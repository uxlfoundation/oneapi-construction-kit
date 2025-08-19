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

#include "mux/hal/memory.h"

#include "mux/utils/allocator.h"

namespace mux {
namespace hal {
memory::memory(uint64_t size, uint32_t properties, ::hal::hal_addr_t targetPtr,
               void *hostPtr)
    : targetPtr(targetPtr), hostPtr(hostPtr) {
  this->size = size;
  this->properties = properties;
  this->handle = static_cast<uint64_t>(targetPtr);
}

// muxMapMemory
cargo::expected<void *, mux_result_t> memory::map(::hal::hal_device_t *device,
                                                  uint64_t offset,
                                                  uint64_t size) {
  (void)device;
  if (hostPtr) {
    return static_cast<unsigned char *>(hostPtr) + offset;
  }
  if (cargo::success != mappedMemory.alloc(size)) {
    return cargo::make_unexpected(mux_error_out_of_memory);
  }
  mapOffset = offset;
  return mappedMemory.data();
}

// muxFlushMappedMemoryToDevice
mux_result_t memory::flushToDevice(::hal::hal_device_t *device, uint64_t offset,
                                   uint64_t size) {
  const uint8_t *src = nullptr;
  if (hostPtr) {
    src = static_cast<uint8_t *>(hostPtr) + offset;
  } else {
    if (mappedMemory.empty()) {
      return mux_error_failure;
    }
    src = mappedMemory.data() + offset - mapOffset;
  }
  if (!device->mem_write(targetPtr + offset, src, size)) {
    return mux_error_failure;
  }
  return mux_success;
}

// muxFlushMappedMemoryFromDevice
mux_result_t memory::flushFromDevice(::hal::hal_device_t *device,
                                     uint64_t offset, uint64_t size) {
  uint8_t *dst = nullptr;
  if (hostPtr) {
    dst = static_cast<uint8_t *>(hostPtr) + offset;
  } else {
    if (mappedMemory.empty()) {
      return mux_error_failure;
    }
    dst = static_cast<uint8_t *>(mappedMemory.data()) + offset - mapOffset;
  }
  if (!device->mem_read(dst, targetPtr + offset, size)) {
    return mux_error_failure;
  }
  return mux_success;
}

// muxUnmapMemory
mux_result_t memory::unmap(::hal::hal_device_t *device) {
  (void)device;
  mappedMemory.clear();
  hostPtr = nullptr;
  return mux_success;
}
}  // namespace hal
}  // namespace mux
