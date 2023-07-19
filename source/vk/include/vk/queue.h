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

#ifndef VK_QUEUE_H_INCLUDED
#define VK_QUEUE_H_INCLUDED

#include <mux/mux.h>
#include <vk/error.h>
#include <vk/icd.h>
#include <vk/small_vector.h>
#include <vulkan/vulkan.h>

#include <mutex>
#include <unordered_set>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t *device;

/// @copydoc ::vk::fence_t
typedef struct fence_t *fence;

/// @copydoc ::vk::semaphore_t;
typedef struct semaphore_t *semaphore;

/// @copydoc ::vk::command_buffer_t;
typedef struct command_buffer_t *command_buffer;

/// @copydoc ::vk::queue_t
typedef struct queue_t *queue;

/// @brief Struct which is used as userdata for muxDispatch callback.
///        This is required so that certain Vulkan API features such as
///        command buffer state can be implemented.
typedef struct dispatch_callback_data_s {
  /// @brief The queue our mux command buffer was submitted to
  vk::queue queue;

  /// @brief The command buffer referenced by the submission
  vk::command_buffer commandBuffer;

  /// @brief The semaphore this mux command buffer will signal when it's done
  mux_semaphore_t semaphore;

  /// @brief Stage mask denoting which stage this mux command buffer executed in
  ///
  /// used to figure out which lists of semaphores we need to remove our
  /// `semaphore` from
  VkPipelineStageFlags stage_flags;

  /// @brief Constructor
  ///
  /// @param queue The queue that the command buffer was submitted to
  /// @param commandBuffer `command_buffer` handle to be stored inside the
  ///        struct
  /// @param semaphore Semaphore the submitted mux command buffer signals
  /// @param stage_flags Stage flags denoting which stages the mux command
  /// buffer
  ///        runs in
  dispatch_callback_data_s(vk::queue queue, vk::command_buffer commandBuffer,
                           mux_semaphore_t semaphore,
                           VkPipelineStageFlags stage_flags);
} *dispatch_callback_data;

/// @brief internal queue type
typedef struct queue_t final : icd_t<queue_t> {
  /// @brief Constructor
  ///
  /// @param mux_queue Instance of mux_queue_t to take ownership of
  /// @param allocator Allocator retained for allocations in `QueueSubmit`
  queue_t(mux_queue_t mux_queue, vk::allocator allocator);

  /// @brief Destructor
  ~queue_t();

  /// @brief Instance of mux_queue_t that this object was created with
  mux_queue_t mux_queue;

  /// @brief `vk::allocator` passed to `vkCreateDevice`, stored here so it can
  /// be used for allocations in queue related functions
  vk::allocator allocator;

  /// @brief Mutex used for locking during the submit callback
  std::mutex mutex;

  /// @brief List of semaphores that will be signaled by executing command
  /// groups with compute work enqueued to them
  std::unordered_set<mux_semaphore_t> compute_waits;

  /// @brief List of semaphores that will be signaled by executing command
  /// groups with transfer work enqueued to them
  std::unordered_set<mux_semaphore_t> transfer_waits;

  /// @brief List of user generated semaphores that compute commands wait for
  std::unordered_set<mux_semaphore_t> user_compute_waits;

  /// @brief List of user generated semaphores that transfer commands wait for
  std::unordered_set<mux_semaphore_t> user_transfer_waits;

  /// @brief Mux command buffer used to signal fences submitted to the queue
  mux_command_buffer_t fence_command_buffer;

  /// @brief List of fence mux command buffers that had to be replaced
  ///
  /// because they were still waiting for their mux command buffers when another
  /// fence was enqueued. Stored here so they can be cleaned up when the queue
  /// is destroyed.
  vk::small_vector<mux_command_buffer_t, 2> fence_command_buffers;

  /// @brief Whether `fence_command_buffer` has ever been submitted to a queue
  ///
  /// This is needed so we don't call `muxTryWait` on a newly created
  /// `fence_command_buffer` and end up erroneously waiting for it to finish
  /// because `mux_fence_not_ready` will return from both a
  /// mux command buffer in progress and a mux command buffer that was never
  /// submitted
  bool fence_submitted;
} *queue;

/// @brief internal implementation of vkGetDeviceQueue
///
/// @param device The device that owns the queue
/// @param queueFamilyIndex The index of the queue family to retrieve a queue
/// from
/// @param queueIndex The index of queue to retrieve from the queue family
void GetDeviceQueue(vk::device device, uint32_t queueFamilyIndex,
                    uint32_t queueIndex, vk::queue *pQueue);

/// @brief internal implementation of vkQueueWaitIdle
///
/// @param queue the queue to wait on
///
/// @return return Vulkan result code
VkResult QueueWaitIdle(vk::queue queue);

/// @brief internal implementation of vkQueueSubmit
///
/// @param queue queue to submit to
/// @param submitCount length of pSubmits
/// @param pSubmits array of VkSubmitInfo structs detailing each workload to be
/// submitted
/// @param fence optional handle to a fence which is to be signaled
VkResult QueueSubmit(vk::queue queue, uint32_t submitCount,
                     const VkSubmitInfo *pSubmits, vk::fence fence);

/// @brief Stub of vkQueueBindSparse
VkResult QueueBindSparse(vk::queue queue, uint32_t bindInfoCount,
                         const VkBindSparseInfo *pBindInfo, vk::fence fence);
}  // namespace vk

#endif  // VK_QUEUE_H_INCLUDED
