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

#ifndef VK_COMMAND_POOL_H_INCLUDED
#define VK_COMMAND_POOL_H_INCLUDED

#include <vk/small_vector.h>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t *device;

/// @copydoc ::vk::command_buffer_t
typedef struct command_buffer_t *command_buffer;

/// @brief internal commandPool type
typedef struct command_pool_t final {
  /// @brief Constructor
  ///
  /// @param flags command_pool create flags
  /// @param queueFamilyIndex queue family to which all queues created from
  /// this pool must be submitted
  /// @param allocator `vk::allocator` used to allocate command buffers
  command_pool_t(VkCommandPoolCreateFlags flags, uint32_t queueFamilyIndex,
                 vk::allocator allocator);

  /// @brief Destructor
  ~command_pool_t();

  /// @brief every command buffer created from the pool will have a reference
  /// stored here, so they can all be reset with a ResetCommandPool call
  vk::small_vector<command_buffer, 4> command_buffers;

  /// @brief command pool flags provided at creation
  VkCommandPoolCreateFlags flags;

  /// @brief queueFamilyIndex provided at creation
  uint32_t queueFamilyIndex;

  /// @brief currently the allocator used to create the object, later this will
  /// be an allocator which is only capable of allocating from the pool
  vk::allocator allocator;
} * command_pool;

/// @brief internal implementation of vkCreateCommandPool
///
/// @param device device on which to create the command pool
/// @param pCreateInfo create info
/// @param allocator allocator
/// @param pCommandPool return created command pool
///
/// @return Vulkan result code
VkResult CreateCommandPool(vk::device device,
                           const VkCommandPoolCreateInfo *pCreateInfo,
                           vk::allocator allocator,
                           vk::command_pool *pCommandPool);

/// @brief internal implementation of VkDestroyCommandPool
///
/// @param device Device the command pool was created on
/// @param commandPool command pool to destroy
/// @param allocator allocator
void DestroyCommandPool(vk::device device, vk::command_pool commandPool,
                        vk::allocator allocator);

/// @brief internal implementation of vkResetCommandPool
///
/// @param device device the command pool was created on
/// @param commandPool the command pool to be reset
/// @param flags flags which control the behavior of the reset
///
/// @return Vulkan result code
VkResult ResetCommandPool(vk::device device, vk::command_pool commandPool,
                          VkCommandPoolResetFlags flags);
}  // namespace vk

#endif  // VK_COMMAND_POOL_H_INCLUDED
