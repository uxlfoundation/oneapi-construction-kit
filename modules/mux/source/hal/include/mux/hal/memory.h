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

/// @file
///
/// @brief HAL base implementation of the mux_memory_s object.

#ifndef MUX_HAL_MEMORY_H_INCLUDED
#define MUX_HAL_MEMORY_H_INCLUDED

#include "cargo/dynamic_array.h"
#include "cargo/expected.h"
#include "hal.h"
#include "mux/mux.h"
#include "mux/utils/allocator.h"

namespace mux {
namespace hal {
struct memory : mux_memory_s {
  enum heap_e : uint32_t {
    HEAP_ALL = 0x1 << 0,
    HEAP_BUFFER = 0x1 << 1,
    HEAP_IMAGE = 0x1 << 2,
  };

  memory(uint64_t size, uint32_t properties, ::hal::hal_addr_t data,
         void *origHostPtr);

  /// @see muxAllocateMemory
  template <class Memory>
  static cargo::expected<Memory *, mux_result_t> create(
      ::hal::hal_device_t *device, size_t size, uint32_t heap,
      uint32_t memory_properties, mux_allocation_type_e allocation_type,
      uint32_t alignment, mux::allocator allocator) {
    static_assert(std::is_base_of_v<mux::hal::memory, Memory>,
                  "template type Memory must derive from mux::hal::memory");
    (void)allocation_type;
    // Ensure the specified heap is valid, as heaps are target specific the
    // check must be performed by the target.
    switch (heap) {
      case memory::HEAP_ALL:
      case memory::HEAP_BUFFER:
      case memory::HEAP_IMAGE:
        break;
      default:
        return cargo::make_unexpected(mux_error_invalid_value);
    }
    // Align all allocations to at least 128 bytes to match the size of the
    // largest 16-wide OpenCL-C vector types.
    const uint32_t align = std::max<uint32_t>(alignment, 128u);
    const ::hal::hal_addr_t target_ptr = device->mem_alloc(size, align);
    if (::hal::hal_nullptr == target_ptr) {
      return cargo::make_unexpected(mux_error_out_of_memory);
    }
    auto memory = allocator.create<Memory>(size, memory_properties, target_ptr,
                                           /*origHostPtr*/ nullptr);
    if (!memory) {
      device->mem_free(target_ptr);
      return cargo::make_unexpected(mux_error_out_of_memory);
    }
    return memory;
  }

  /// @see muxCreateMemoryFromHost
  template <class Memory>
  static cargo::expected<Memory *, mux_result_t> create(
      ::hal::hal_device_t *device, size_t size, void *pointer,
      mux::allocator allocator) {
    static_assert(std::is_base_of_v<mux::hal::memory, Memory>,
                  "template type Memory must derive from mux::hal::memory");
    (void)device;
    (void)size;
    (void)pointer;
    (void)allocator;
    return cargo::make_unexpected(mux_error_feature_unsupported);
  }

  /// @see muxFreeMemory
  template <class Memory>
  static void destroy(::hal::hal_device_t *device, Memory *memory,
                      mux::allocator allocator) {
    static_assert(std::is_base_of_v<mux::hal::memory, Memory>,
                  "template type Memory must derive from mux::hal::memory");
    if (memory->targetPtr != ::hal::hal_nullptr) {
      device->mem_free(memory->targetPtr);
      memory->targetPtr = ::hal::hal_nullptr;
    }
    allocator.destroy(memory);
  }

  /// @see muxMapMemory
  cargo::expected<void *, mux_result_t> map(::hal::hal_device_t *device,
                                            uint64_t offset, uint64_t size);

  /// @see muxFlushMappedMemoryToDevice
  mux_result_t flushToDevice(::hal::hal_device_t *device, uint64_t offset,
                             uint64_t size);

  /// @see muxFlushMappedMemoryFromDevice
  mux_result_t flushFromDevice(::hal::hal_device_t *device, uint64_t offset,
                               uint64_t size);

  /// @see muxUnmapMemory
  mux_result_t unmap(::hal::hal_device_t *device);

  /// @brief Pointer to device memory
  ::hal::hal_addr_t targetPtr;
  /// @brief Pointer to memory of the CA host
  void *hostPtr;
  uint64_t mapOffset;
  cargo::dynamic_array<uint8_t> mappedMemory;
};
}  // namespace hal
}  // namespace mux

#endif  // MUX_HAL_MEMORY_H_INCLUDED
