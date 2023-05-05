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

#ifndef VK_FENCE_H_INCLUDED
#define VK_FENCE_H_INCLUDED

#include <mux/mux.hpp>
#include <vk/allocator.h>

#include <condition_variable>
#include <mutex>
#include <vector>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t *device;

typedef struct fence_t final {
  /// @brief Constructor
  ///
  /// @param signaled The fence's state, determined by the flags passed in
  /// createInfo
  /// @param command_buffer The mux command buffer that will signal this fence
  /// @param mux_fence The underlying mux fence that will signal this fence
  fence_t(bool signaled, mux::unique_ptr<mux_command_buffer_t> &&command_buffer,
          mux::unique_ptr<mux_fence_t> mux_fence);

  /// @brief The fence's state
  ///
  /// We need a signal here because it's possible to create vk fences in the
  /// signaled state.
  bool signaled;
  /// @brief Mutex for the signaled bool state.
  ///
  /// We still need to protect concurrent access of `signaled` since it is
  /// essentially a cache of the last queried fence state which can be
  /// initialized to signaled on fence creation..
  std::mutex signaled_mutex;
  /// @brief Mux command buffer used to signal this fence
  mux_command_buffer_t command_buffer;
  /// @brief Mux fence object used for device -> host synchronization.
  mux_fence_t mux_fence;
} * fence;

/// @brief Internal implementation of vkCreateFence
///
/// @param device The fence is to be created on
/// @param pCreateInfo Create info
/// @param allocator Allocator used to allocate the fence's memory
/// @param pFence Return created fence
///
/// @return Vulkan result code
VkResult CreateFence(vk::device device, const VkFenceCreateInfo *pCreateInfo,
                     vk::allocator allocator, vk::fence *pFence);

/// @brief Internal implementation of vkDestroyFence
///
/// @param device Device the fence was created on
/// @param fence The fence to destroy
void DestroyFence(vk::device device, vk::fence fence, vk::allocator allocator);

/// @brief Internal implementation of vkGetFenceStatus
///
/// @param device The device the fence was created on
/// @param fence The fence to query
///
/// @return Vulkan result code
VkResult GetFenceStatus(vk::device device, vk::fence fence);

/// @brief Internal implementation of vkResetFences
///
/// @param device Device that owns the fence(s)
/// @param fenceCount Length of `pFences`
/// @param pFences List of fences to reset
///
/// @return Vulkan result code
VkResult ResetFences(vk::device device, uint32_t fenceCount,
                     const vk::fence *pFences);

/// @brief Internal implementation of vkWaitForFences
///
/// @param device Device that owns the fences
/// @param fenceCount Length of `pFences`
/// @param pFences Pointer to array of fences to wait on
/// @param waitAll If true wait on all fences, otherwise only wait for one
/// @param timeout Timeout value in nanoseconds
VkResult WaitForFences(vk::device device, uint32_t fenceCount,
                       const VkFence *pFences, VkBool32 waitAll,
                       uint64_t timeout);

/// @brief User callback pushed to fence mux command buffers that signals the
/// fence
void fenceSignalCallback(mux_queue_t, mux_command_buffer_t, void *user_data);
}  // namespace vk

#endif  // VK_FENCE_H_INCLUDED

