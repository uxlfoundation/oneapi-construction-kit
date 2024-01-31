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
#include <vk/fence.h>
#include <vk/queue.h>
#include <vk/small_vector.h>
#include <vk/type_traits.h>
#include <vulkan/vulkan_core.h>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>

#include "mux/mux.h"
#include "mux/mux.hpp"
#include "vk/error.h"

namespace vk {
fence_t::fence_t(bool signaled,
                 mux::unique_ptr<mux_command_buffer_t> &&command_buffer,
                 mux::unique_ptr<mux_fence_t> mux_fence)
    : signaled(signaled),
      command_buffer(command_buffer.release()),
      mux_fence(mux_fence.release()) {}

VkResult CreateFence(vk::device device, const VkFenceCreateInfo *pCreateInfo,
                     vk::allocator allocator, vk::fence *pFence) {
  mux_command_buffer_t command_buffer;

  if (auto error = muxCreateCommandBuffer(device->mux_device, nullptr,
                                          allocator.getMuxAllocator(),
                                          &command_buffer)) {
    return vk::getVkResult(error);
  }
  mux::unique_ptr<mux_command_buffer_t> command_buffer_ptr(
      command_buffer, {device->mux_device, allocator.getMuxAllocator()});

  mux_fence_t mux_fence = nullptr;
  if (auto error = muxCreateFence(device->mux_device,
                                  allocator.getMuxAllocator(), &mux_fence)) {
    return vk::getVkResult(error);
  }

  vk::fence fence = allocator.create<vk::fence_t>(
      VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE,
      (pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT),
      std::move(command_buffer_ptr),
      mux::unique_ptr<mux_fence_t>{
          mux_fence, {device->mux_device, allocator.getMuxAllocator()}});

  if (!fence) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  *pFence = fence;

  return VK_SUCCESS;
}

void DestroyFence(vk::device device, vk::fence fence, vk::allocator allocator) {
  if (fence == VK_NULL_HANDLE) {
    return;
  }

  muxDestroyCommandBuffer(device->mux_device, fence->command_buffer,
                          allocator.getMuxAllocator());
  muxDestroyFence(device->mux_device, fence->mux_fence,
                  allocator.getMuxAllocator());
  allocator.destroy(fence);
}

VkResult GetFenceStatus(vk::device device, vk::fence fence) {
  // First see if we are already signaled.
  {
    const std::unique_lock<std::mutex> lock(fence->signaled_mutex);
    if (fence->signaled) {
      return VK_SUCCESS;
    }
  }

  // Try waiting on the fence so we can get its status.
  const auto error = muxTryWait(device->queue->mux_queue, 0, fence->mux_fence);

  if (mux_success == error) {
    // If the fence has been signaled we need to make sure we set the signal.
    {
      const std::unique_lock<std::mutex> lock(fence->signaled_mutex);
      fence->signaled = true;
    }
    return VK_SUCCESS;
  }

  if (mux_fence_not_ready == error) {
    // If the fence isn't signaled then that is fine, we just need to report
    // that to the VK layer.
    return VK_NOT_READY;
  }

  // Otherwise something has gone and we are in trouble.
  return vk::getVkResult(error);
}

VkResult ResetFences(vk::device device, uint32_t fenceCount,
                     const vk::fence *pFences) {
  (void)device;
  for (uint32_t fenceIndex = 0; fenceIndex < fenceCount; fenceIndex++) {
    auto *const fence = pFences[fenceIndex];
    // Try reseting the underlying mux fence.
    if (auto error = muxResetFence(fence->mux_fence)) {
      return vk::getVkResult(error);
    }
    // If that was successful then we can reset the signal
    {
      const std::unique_lock<std::mutex> lock(fence->signaled_mutex);
      fence->signaled = false;
    }
  }
  // If we got this far we successfully reset all the fences.
  return VK_SUCCESS;
}

VkResult WaitForFences(vk::device device, uint32_t fenceCount,
                       const VkFence *pFences, VkBool32 waitAll,
                       uint64_t timeout) {
  // If waitAll is false then we only need one fence to signal before we return.
  // In this case to avoid disproportionately waiting on the first fence we
  // scale the timeout so each fence gets a fair chance of being the first to
  // signal.
  const uint64_t scaledTimeout =
      (VK_TRUE == waitAll) ? timeout
                           : timeout / static_cast<uint64_t>(fenceCount);
  const auto start = std::chrono::steady_clock::now();
  for (uint32_t fenceIndex = 0; fenceIndex < fenceCount; fenceIndex++) {
    vk::fence fence = vk::cast<vk::fence>(pFences[fenceIndex]);

    do {
      // Try waiting on the underlying mux fence.
      const auto error =
          muxTryWait(device->queue->mux_queue, scaledTimeout, fence->mux_fence);
      const auto end = std::chrono::steady_clock::now();

      // Then see if we timed out during that wait.
      const auto time_waited =
          std::chrono::duration<uint64_t, std::nano>(end - start);
      if (static_cast<uint64_t>(time_waited.count()) > timeout) {
        return VK_TIMEOUT;
      }

      // If the fence was signaled then we can move on or exit early in the
      // case we only need one fence to signal.
      if (mux_success == error) {
        {
          const std::unique_lock<std::mutex> lock(fence->signaled_mutex);
          fence->signaled = true;
        }
        if (VK_TRUE == waitAll) {
          break;
        } else {
          return VK_SUCCESS;
        }
      }

      // If there is a genuine error we need communicate this to VK.
      if (mux_fence_not_ready != error) {
        return vk::getVkResult(error);
      }
    } while (true);
  }

  // If we got here we successfully waited on all fences.
  return VK_SUCCESS;
}
}  // namespace vk
