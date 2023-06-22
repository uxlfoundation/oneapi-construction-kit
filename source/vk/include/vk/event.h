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

#ifndef VK_EVENT_H_INCLUDED
#define VK_EVENT_H_INCLUDED

#include <vk/allocator.h>
#include <vk/small_vector.h>
#include <vulkan/vulkan.h>

#include <condition_variable>
#include <memory>
#include <mutex>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t *device;

/// @brief Struct passed as user data to the wait events user callback command
typedef struct wait_callback_data_s {
  /// @brief Mutex used by `condition_variable` to lock its thread
  std::mutex mutex;

  /// @brief Condition variable used to wait for all the set event operations
  std::condition_variable condition_variable;

  /// @brief Counter that is decremented when a set event notifies
  uint32_t event_count;

  /// @brief Allocator used to create this object
  vk::allocator *allocator;

  /// @brief Constructor
  wait_callback_data_s(vk::allocator *allocator)
      : mutex(), condition_variable(), event_count(0), allocator(allocator) {}
} *wait_callback_data;

/// @brief internal implementation of VkEvent
typedef struct event_t final {
  /// @brief constructor
  event_t(vk::allocator allocator);

  /// @brief destructor
  ~event_t();

  /// @brief The event's state
  bool signaled;

  /// @brief Set to the stage mask of a set event command that uses this event
  VkPipelineStageFlags set_stage;

  /// @brief Mutex for controlling access to `signaled`
  std::mutex mutex;

  /// @brief List of `wait_callback_data` structs representing the wait events
  /// commands that will wait on this event
  vk::small_vector<wait_callback_data, 2> wait_infos;
} *event;

/// @brief internal implementation of vkCreateEvent
///
/// @param device the device on which the event will be created
/// @param pCreateInfo event create info
/// @param allocator the allocator that will be used to create the event
/// @param pEvent return created event
///
/// @return Vulkan result code
VkResult CreateEvent(vk::device device, const VkEventCreateInfo *pCreateInfo,
                     vk::allocator allocator, vk::event *pEvent);

/// @brief internal implementation of vkDestroyEvent
///
/// @param device the device the event was created on
/// @param event the event to be destroyed
/// @param allocator the allocator that was used to create the event
void DestroyEvent(vk::device device, vk::event event, vk::allocator allocator);

/// @brief internal implementation of vkGetEventStatus
///
/// @param device the device the event was created on
/// @param event the event to query for status
///
/// @return Vulkan result code
VkResult GetEventStatus(vk::device device, vk::event event);

/// @brief internal implementation of vkSetEvent
///
/// @param device the device the event was created on
/// @param event the event to set signaled
///
/// @return Vulkan result code
VkResult SetEvent(vk::device device, vk::event event);

/// @brief internal implementation of vkResetEvent
///
/// @param device the device the event was created on
/// @param event the event to reset
///
/// @return Vulkan result code
VkResult ResetEvent(vk::device device, vk::event event);

/// @brief User callback for the `muxCommandUserCallback` in `CmdSetEvent`
void setEventCallback(mux_queue_t, mux_command_buffer_t, void *user_data);

/// @brief User callback for the `muxCommandUserCallback` in `CmdResetEvent`
void resetEventCallback(mux_queue_t, mux_command_buffer_t, void *user_data);

/// @brief User callback for the `muxCommandUserCallback` in `CmdWaitEvents`
void waitEventCallback(mux_queue_t, mux_command_buffer_t, void *user_data);
}  // namespace vk

#endif  // VK_EVENT_H_INCLUDED
