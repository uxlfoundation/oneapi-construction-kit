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

#ifndef VK_DEVICE_MEMORY_H_INCLUDED
#define VK_DEVICE_MEMORY_H_INCLUDED

#include <mux/mux.h>
#include <vk/allocator.h>
#include <vk/error.h>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t *device;

/// @copydoc ::vk::buffer_t
typedef struct buffer_t *buffer;

/// @brief Internal device_memory type
typedef struct device_memory_t final {
  /// @brief Constructor
  ///
  /// @param mux_memory Mux memory object to take ownership of
  device_memory_t(mux_memory_t mux_memory);

  /// @brief Destructor
  ~device_memory_t();

  /// @brief Mux memory
  mux_memory_t mux_memory;
} *device_memory;

/// @brief Internal implementation of vkAllocatorMemory
///
/// @param device Device on which to allocate memory
/// @param pAllocateInfo Allocate info
/// @param allocator Allocator
/// @param pMemory Return handle to allocated memory
///
/// @return return Vulkan result code
VkResult AllocateMemory(vk::device device,
                        const VkMemoryAllocateInfo *pAllocateInfo,
                        vk::allocator allocator, vk::device_memory *pMemory);

/// @brief internal implementation of vkFreeMemory
///
/// @param device Device on which the memory was allocated
/// @param memory The memory to free
/// @param allocator The allocator used to create the memory object
void FreeMemory(vk::device device, vk::device_memory memory,
                vk::allocator allocator);

/// @brief internal implementation of vkBindBufferMemory
///
/// @param device Device that both the buffer and memory were created on.
/// @param buffer The buffer to bind allocated memory to.
/// @param memory The handle to the block of allocated memory to be bound.
/// @param memoryOffset Offset into the memory to be bound.
VkResult BindBufferMemory(vk::device device, vk::buffer buffer,
                          vk::device_memory memory, VkDeviceSize memoryOffset);

/// @brief Stub of vkBindImageMemory
VkResult BindImageMemory(vk::device device, VkImage image,
                         vk::device_memory memory, VkDeviceSize memoryOffset);

/// @brief Internal implementation of vkMapMemory
///
/// @param device Device the memory to map has been allocated on.
/// @param memory Handle to the allocated memory.
/// @param offset Offset into the allocated memory.
/// @param size The range of memory to map, or VK_WHOLE_SIZE for the full range.
/// @param flags Reserved for future use, must be zero.
/// @param ppData Pointer to host memory that the memory will be mapped to.
///
/// @return Return Vulkan result code
VkResult MapMemory(vk::device device, vk::device_memory memory,
                   VkDeviceSize offset, VkDeviceSize size,
                   VkMemoryMapFlags flags, void **ppData);

/// @brief Internal implementation of vkUnmapMemory.
///
/// @param device Device the memory was allocated on
/// @param device_memory Handle to the memory which will be unmapped
void UnmapMemory(vk::device device, vk::device_memory device_memory);

/// @brief Internal implementation of vkFlushMappedMemoryRanges.
///
/// @param device Device is the logical device of which the memory ranges belong
/// @param memoryRangeCount is the length of the pMemoryRanges array
/// @param pMemoryRanges is a pointer to an array of VkMappedMemoryRange
/// structures describing the memory ranges to
/// flush

VkResult FlushMemoryMappedRanges(vk::device device, uint32_t memoryRangeCount,
                                 const VkMappedMemoryRange *pMemoryRanges);

/// @brief Internal implementation of vkInvalidateMappedMemoryRanges.
///
/// @param device Device is the logical device of which the memory ranges belong
/// @param memoryRangeCount is the length of the pMemoryRanges array
/// @param pMemoryRanges is a pointer to an array of VkMappedMemoryRange
/// structures describing the memory ranges to
/// invalidate
VkResult InvalidateMemoryMappedRanges(vk::device device,
                                      uint32_t memoryRangeCount,
                                      const VkMappedMemoryRange *pMemoryRanges);

}  // namespace vk

#endif  // VK_DEVICE_MEMORY_H_INCLUDED
