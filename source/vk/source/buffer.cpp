// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <vk/buffer.h>
#include <vk/device.h>

#include <utility>

namespace vk {
buffer_t::buffer_t(mux::unique_ptr<mux_buffer_t> &&mux_buffer,
                   VkBufferUsageFlags usage)
    : mux_buffer(mux_buffer.release()), usage(usage) {}

buffer_t::~buffer_t() {}

VkResult CreateBuffer(vk::device device, const VkBufferCreateInfo *pCreateInfo,
                      vk::allocator allocator, vk::buffer *pBuffer) {
  mux_buffer_t mux_buffer;
  mux_result_t error =
      muxCreateBuffer(device->mux_device, pCreateInfo->size,
                      allocator.getMuxAllocator(), &mux_buffer);

  if (error) {
    return vk::getVkResult(error);
  }

  mux::unique_ptr<mux_buffer_t> mux_buffer_ptr(
      mux_buffer, {device->mux_device, allocator.getMuxAllocator()});

  vk::buffer out_buffer = allocator.create<vk::buffer_t>(
      VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, std::move(mux_buffer_ptr),
      pCreateInfo->usage);

  if (!out_buffer) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  *pBuffer = out_buffer;

  return VK_SUCCESS;
}

void DestroyBuffer(vk::device device, vk::buffer buffer,
                   vk::allocator allocator) {
  if (buffer == VK_NULL_HANDLE) {
    return;
  }

  muxDestroyBuffer(device->mux_device, buffer->mux_buffer,
                   allocator.getMuxAllocator());
  allocator.destroy(buffer);
}

void GetBufferMemoryRequirements(vk::device device, vk::buffer buffer,
                                 VkMemoryRequirements *pMemoryRequirements) {
  VkMemoryRequirements memory_requirements = {};
  memory_requirements.alignment = device->mux_device->info->buffer_alignment;

  uint64_t buffer_size = buffer->mux_buffer->memory_requirements.size;

  if (buffer_size <= memory_requirements.alignment) {
    memory_requirements.size = memory_requirements.alignment;
  } else {
    // round the size of the buffer to the next multiple of alignment
    memory_requirements.size = (memory_requirements.alignment *
                                (buffer_size / memory_requirements.alignment)) +
                               memory_requirements.alignment % buffer_size;
  }

  for (uint32_t typeIndex = 0;
       typeIndex < device->memory_properties.memoryTypeCount; typeIndex++) {
    if (!(device->memory_properties.memoryTypes[typeIndex].propertyFlags &
          VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)) {
      memory_requirements.memoryTypeBits |= 1 << typeIndex;
    }
  }

  *pMemoryRequirements = memory_requirements;
}
}  // namespace vk
