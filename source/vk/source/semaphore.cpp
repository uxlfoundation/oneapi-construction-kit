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

#include <vk/device.h>
#include <vk/semaphore.h>
#include <vk/unique_ptr.h>

#include <utility>

namespace vk {
semaphore_t::semaphore_t(mux::unique_ptr<mux_semaphore_t> &&mux_semaphore,
                         mux::unique_ptr<mux_command_buffer_t> &&command_buffer,
                         mux::unique_ptr<mux_fence_t> fence,
                         vk::allocator allocator)
    : mux_semaphore(mux_semaphore.release()),
      command_buffer(command_buffer.release()),
      mux_fence(fence.release()),
      queue(VK_NULL_HANDLE),
      wait_stage(0),
      has_dispatched(false),
      wait_semaphores(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      semaphore_tuples(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}) {}

semaphore_t::~semaphore_t() {}

VkResult CreateSemaphore(vk::device device,
                         const VkSemaphoreCreateInfo *pCreateInfo,
                         vk::allocator allocator, vk::semaphore *pSemaphore) {
  (void)pCreateInfo;
  mux_semaphore_t mux_semaphore;
  if (auto error = muxCreateSemaphore(
          device->mux_device, allocator.getMuxAllocator(), &mux_semaphore)) {
    return vk::getVkResult(error);
  }
  mux::unique_ptr<mux_semaphore_t> mux_semaphore_ptr(
      mux_semaphore, {device->mux_device, allocator.getMuxAllocator()});

  mux_command_buffer_t command_buffer;
  if (auto error = muxCreateCommandBuffer(device->mux_device, nullptr,
                                          allocator.getMuxAllocator(),
                                          &command_buffer)) {
    return vk::getVkResult(error);
  }
  mux::unique_ptr<mux_command_buffer_t> mux_command_buffer_ptr(
      command_buffer, {device->mux_device, allocator.getMuxAllocator()});

  mux_fence_t mux_fence;
  if (auto error = muxCreateFence(device->mux_device,
                                  allocator.getMuxAllocator(), &mux_fence)) {
    return vk::getVkResult(error);
  }
  mux::unique_ptr<mux_fence_t> mux_fence_ptr(
      mux_fence, {device->mux_device, allocator.getMuxAllocator()});

  vk::semaphore semaphore = allocator.create<vk::semaphore_t>(
      VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, std::move(mux_semaphore_ptr),
      std::move(mux_command_buffer_ptr), std::move(mux_fence_ptr), allocator);

  if (!semaphore) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }
  vk::unique_ptr<vk::semaphore> semaphore_ptr(semaphore, allocator);

  if (semaphore_ptr->semaphore_tuples.push_back({semaphore_ptr->mux_semaphore,
                                                 semaphore->command_buffer,
                                                 semaphore->mux_fence})) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  *pSemaphore = semaphore_ptr.release();

  return VK_SUCCESS;
}

void DestroySemaphore(vk::device device, vk::semaphore semaphore,
                      vk::allocator allocator) {
  if (semaphore == VK_NULL_HANDLE) {
    return;
  }

  for (auto &tuple : semaphore->semaphore_tuples) {
    muxDestroySemaphore(device->mux_device, tuple.semaphore,
                        allocator.getMuxAllocator());
    muxDestroyCommandBuffer(device->mux_device, tuple.command_buffer,
                            allocator.getMuxAllocator());
    muxDestroyFence(device->mux_device, tuple.fence,
                    allocator.getMuxAllocator());
  }

  allocator.destroy<vk::semaphore_t>(semaphore);
}
}  // namespace vk
