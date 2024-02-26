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

#include <vk/command_buffer.h>
#include <vk/command_pool.h>
#include <vk/device.h>

namespace vk {
command_pool_t::command_pool_t(VkCommandPoolCreateFlags flags,
                               uint32_t queueFamilyIndex,
                               vk::allocator allocator)
    : command_buffers(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      flags(flags),
      queueFamilyIndex(queueFamilyIndex),
      allocator(allocator) {}

command_pool_t::~command_pool_t() {}

VkResult CreateCommandPool(vk::device device,
                           const VkCommandPoolCreateInfo *pCreateInfo,
                           vk::allocator allocator,
                           vk::command_pool *pCommandPool) {
  (void)device;
  vk::command_pool command_pool = allocator.create<vk::command_pool_t>(
      VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, pCreateInfo->flags,
      pCreateInfo->queueFamilyIndex, allocator);

  if (!command_pool) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  *pCommandPool = command_pool;
  return VK_SUCCESS;
}

void DestroyCommandPool(vk::device device, vk::command_pool commandPool,
                        vk::allocator allocator) {
  if (commandPool == VK_NULL_HANDLE) {
    return;
  }

  if (!commandPool->command_buffers.empty()) {
    FreeCommandBuffers(device, commandPool, commandPool->command_buffers.size(),
                       commandPool->command_buffers.data());
  }
  allocator.destroy(commandPool);
}

VkResult ResetCommandPool(vk::device device, vk::command_pool commandPool,
                          VkCommandPoolResetFlags flags) {
  (void)flags;
  for (vk::command_buffer command_buffer : commandPool->command_buffers) {
    command_buffer->descriptor_sets.clear();

    for (auto &barrier_info : command_buffer->barrier_group_infos) {
      muxDestroyCommandBuffer(device->mux_device, barrier_info->command_buffer,
                              commandPool->allocator.getMuxAllocator());
      muxDestroySemaphore(device->mux_device, barrier_info->semaphore,
                          commandPool->allocator.getMuxAllocator());
    }

    command_buffer->barrier_group_infos.clear();

    if (auto error =
            muxResetCommandBuffer(command_buffer->main_command_buffer)) {
      return vk::getVkResult(error);
    }

    if (auto error = muxResetSemaphore(command_buffer->main_semaphore)) {
      return vk::getVkResult(error);
    }

    command_buffer->error = VK_SUCCESS;
    command_buffer->state = command_buffer_t::initial;
  }
  return VK_SUCCESS;
}
}  // namespace vk
