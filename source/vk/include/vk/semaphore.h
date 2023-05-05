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

#ifndef VK_SEMAPHORE_H_INCLUDED
#define VK_SEMAPHORE_H_INCLUDED

#include <mux/mux.hpp>
#include <vk/allocator.h>
#include <vk/error.h>
#include <vk/small_vector.h>

#include <mutex>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t *device;

typedef struct queue_t *queue;

/// @brief Semaphore/mux command buffer/fence tuple struct
struct semaphore_command_buffer_fence_tuple {
  mux_semaphore_t semaphore;
  mux_command_buffer_t command_buffer;
  mux_fence_t fence;
};

/// @brief internal semaphore type
typedef struct semaphore_t final {
  /// @brief Constructor
  ///
  /// @param mux_semaphore object to take ownership of
  /// @param command_buffer Mux command buffer that will signal `mux_semaphore`
  /// @param fence Mux fence that will signal `mux_semaphore`
  /// @param allocator Allocator for contructing lists
  semaphore_t(mux::unique_ptr<mux_semaphore_t> &&mux_semaphore,
              mux::unique_ptr<mux_command_buffer_t> &&command_buffer,
              mux::unique_ptr<mux_fence_t> fence, vk::allocator allocator);

  /// @brief Destructor
  ~semaphore_t();

  /// @brief Mux semaphore object to be used for semaphore operations
  mux_semaphore_t mux_semaphore;

  /// @brief Mux command buffer that will be used to signal `mux_semaphore`
  mux_command_buffer_t command_buffer;

  /// @brief Mux fence that will be used to signal `mux_semaphore`
  mux_fence_t mux_fence;

  /// @brief When this semaphore is submitted to a queue as a signal, a
  /// reference to the queue will be stored here
  vk::queue queue;

  /// @brief If this semaphore is submitted as a wait, this encodes the wait
  /// stage(s)
  VkPipelineStageFlags wait_stage;

  /// @brief Bool for tracking whether `command_buffer` has been dispatched
  ///
  /// This is needed as `muxTryWait` does not distinguish between a command
  /// group that has not finished executing and a mux command buffer that has
  /// never been dispatched.
  bool has_dispatched;

  /// @brief List of semaphores to wait on before this semaphore can be signaled
  vk::small_vector<mux_semaphore_t, 4> wait_semaphores;

  /// @brief List of semaphore/mux command buffer/fence tuples created by this
  /// semaphore
  vk::small_vector<semaphore_command_buffer_fence_tuple, 2> semaphore_tuples;

  /// @brief Mutex used to control access to semaphore members
  std::mutex mutex;
} * semaphore;

/// @brief internal implementation of vkCreateSemaphore
///
/// @param device Device on which to create the semaphore
/// @param pCreateInfo create info
/// @param allocator allocator
/// @param pSemaphore return created semaphore
///
/// @return return Vulkan result code
VkResult CreateSemaphore(vk::device device,
                         const VkSemaphoreCreateInfo *pCreateInfo,
                         vk::allocator allocator, vk::semaphore *pSemaphore);

/// @brief internal implementation of vkDestroySemaphore
///
/// @param device Device on which the semaphore was created
/// @param semaphore Handle to the semaphore which is to be destroyed
/// @param allocator allocator
void DestroySemaphore(vk::device device, vk::semaphore semaphore,
                      vk::allocator allocator);
}  // namespace vk

#endif  // VK_SEMAPHORE_H_INCLUDED
