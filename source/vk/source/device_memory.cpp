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

#include <vk/buffer.h>
#include <vk/device.h>
#include <vk/device_memory.h>
#include <vk/type_traits.h>

namespace vk {
device_memory_t::device_memory_t(mux_memory_t mux_memory)
    : mux_memory(mux_memory) {}

device_memory_t::~device_memory_t() {}

VkResult AllocateMemory(vk::device device,
                        const VkMemoryAllocateInfo *pAllocateInfo,
                        vk::allocator allocator, vk::device_memory *pMemory) {
  const VkMemoryType type =
      device->memory_properties.memoryTypes[pAllocateInfo->memoryTypeIndex];
  mux_memory_t mux_memory;
  mux_result_t error = mux_success;

  uint32_t memory_properties = 0;

  if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT &&
      !(type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
    memory_properties |= mux_memory_property_device_local;
  } else {
    if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
      memory_properties |= mux_memory_property_host_visible;
    }
    if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
      memory_properties |= mux_memory_property_host_coherent;
    } else if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
      memory_properties |= mux_memory_property_host_cached;
    }
  }

  // the 1 being passed for heap to muxAllocateMemory is derived from the heap
  // enum in host/memory.h where HEAP_ALL is defined as 0x1 << 0, since at this
  // stage vulkan does not discriminate between memory allocations for images
  // and buffers
  if (memory_properties) {
    error =
        muxAllocateMemory(device->mux_device, pAllocateInfo->allocationSize, 1,
                          memory_properties, mux_allocation_type_alloc_host, 0,
                          allocator.getMuxAllocator(), &mux_memory);
  } else {
    VK_ABORT("unsupported memory type property flags")
  }

  if (mux_success != error) {
    return vk::getVkResult(error);
  }

  vk::device_memory memory = allocator.create<vk::device_memory_t>(
      VK_SYSTEM_ALLOCATION_SCOPE_DEVICE, mux_memory);

  if (!memory) {
    muxFreeMemory(device->mux_device, mux_memory, allocator.getMuxAllocator());
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  *pMemory = memory;

  return VK_SUCCESS;
}

void FreeMemory(vk::device device, vk::device_memory memory,
                vk::allocator allocator) {
  if (memory == VK_NULL_HANDLE) {
    return;
  }

  muxFreeMemory(device->mux_device, memory->mux_memory,
                allocator.getMuxAllocator());
  allocator.destroy(memory);
}

VkResult BindBufferMemory(vk::device device, vk::buffer buffer,
                          vk::device_memory memory, VkDeviceSize memoryOffset) {
  const mux_result_t error = muxBindBufferMemory(
      device->mux_device, memory->mux_memory, buffer->mux_buffer, memoryOffset);

  if (error) {
    return vk::getVkResult(error);
  }

  return VK_SUCCESS;
}

VkResult BindImageMemory(vk::device device, VkImage image,
                         vk::device_memory memory, VkDeviceSize memoryOffset) {
  (void)device;
  (void)image;
  (void)memory;
  (void)memoryOffset;
  return VK_ERROR_FEATURE_NOT_PRESENT;
}

VkResult MapMemory(vk::device device, vk::device_memory memory,
                   VkDeviceSize offset, VkDeviceSize size,
                   VkMemoryMapFlags flags, void **ppData) {
  // as noted in the spec this parameter is reserved for future use
  (void)flags;

  if (VK_WHOLE_SIZE == size) {
    size = memory->mux_memory->size - offset;
  }

  const mux_result_t error = muxMapMemory(
      device->mux_device, memory->mux_memory, offset, size, ppData);

  if (mux_success != error) {
    return vk::getVkResult(error);
  }

  return VK_SUCCESS;
}

void UnmapMemory(vk::device device, vk::device_memory device_memory) {
  muxUnmapMemory(device->mux_device, device_memory->mux_memory);
}

VkResult FlushMemoryMappedRanges(vk::device device, uint32_t memoryRangeCount,
                                 const VkMappedMemoryRange *pMemoryRanges) {
  for (uint32_t i = 0; i < memoryRangeCount; i++) {
    mux_memory_t &mux_mem =
        vk::cast<vk::device_memory>(pMemoryRanges[i].memory)->mux_memory;

    const size_t size = pMemoryRanges[i].size == VK_WHOLE_SIZE
                            ? mux_mem->size - pMemoryRanges[i].offset
                            : pMemoryRanges[i].size;

    const mux_result_t err = muxFlushMappedMemoryToDevice(
        device->mux_device, mux_mem, pMemoryRanges[i].offset, size);

    if (err != mux_success) {
      return vk::getVkResult(err);
    }
  }

  return VK_SUCCESS;
}

VkResult InvalidateMemoryMappedRanges(
    vk::device device, uint32_t memoryRangeCount,
    const VkMappedMemoryRange *pMemoryRanges) {
  for (uint32_t i = 0; i < memoryRangeCount; i++) {
    mux_memory_t &mux_mem =
        vk::cast<vk::device_memory>(pMemoryRanges[i].memory)->mux_memory;

    const size_t size = pMemoryRanges[i].size == VK_WHOLE_SIZE
                            ? mux_mem->size - pMemoryRanges[i].offset
                            : pMemoryRanges[i].size;

    const mux_result_t err = muxFlushMappedMemoryFromDevice(
        device->mux_device, mux_mem, pMemoryRanges[i].offset, size);

    if (err != mux_success) {
      return vk::getVkResult(err);
    }
  }

  return VK_SUCCESS;
}

}  // namespace vk
