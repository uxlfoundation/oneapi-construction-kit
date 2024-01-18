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

#include <vk/event.h>

namespace vk {
event_t::event_t(vk::allocator allocator)
    : signaled(false),
      set_stage(0),
      wait_infos(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}) {}

event_t::~event_t() {}

VkResult CreateEvent(vk::device device, const VkEventCreateInfo *pCreateInfo,
                     vk::allocator allocator, vk::event *pEvent) {
  (void)device;
  (void)pCreateInfo;

  vk::event event = allocator.create<vk::event_t>(
      VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, allocator);

  if (!event) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  *pEvent = event;

  return VK_SUCCESS;
}

void DestroyEvent(vk::device, vk::event event, vk::allocator allocator) {
  if (event == VK_NULL_HANDLE) {
    return;
  }

  allocator.destroy(event);
}

VkResult GetEventStatus(vk::device, vk::event event) {
  const std::lock_guard<std::mutex> lock(event->mutex);
  if (event->signaled) {
    return VK_EVENT_SET;
  }
  return VK_EVENT_RESET;
}

VkResult SetEvent(vk::device, vk::event event) {
  const std::lock_guard<std::mutex> lock(event->mutex);

  event->signaled = true;
  event->set_stage = 0;
  if (!event->wait_infos.empty()) {
    for (auto wait_info : event->wait_infos) {
      const std::lock_guard<std::mutex> waitLock(wait_info->mutex);
      wait_info->event_count--;
      wait_info->condition_variable.notify_all();
    }
    event->wait_infos.clear();
  }
  return VK_SUCCESS;
}

VkResult ResetEvent(vk::device, vk::event event) {
  const std::lock_guard<std::mutex> lock(event->mutex);

  event->signaled = false;

  return VK_SUCCESS;
}

void setEventCallback(mux_queue_t, mux_command_buffer_t, void *user_data) {
  vk::event event = reinterpret_cast<vk::event>(user_data);
  const std::lock_guard<std::mutex> lock(event->mutex);
  event->signaled = true;
  if (!event->wait_infos.empty()) {
    for (auto wait_info : event->wait_infos) {
      const std::lock_guard<std::mutex> waitLock(wait_info->mutex);
      wait_info->event_count--;
      wait_info->condition_variable.notify_all();
    }
    event->wait_infos.clear();
  }
}

void resetEventCallback(mux_queue_t, mux_command_buffer_t, void *user_data) {
  vk::event event = reinterpret_cast<vk::event>(user_data);
  const std::lock_guard<std::mutex> lock(event->mutex);
  event->signaled = false;
}

void waitEventCallback(mux_queue_t, mux_command_buffer_t, void *user_data) {
  auto wait_info = reinterpret_cast<wait_callback_data>(user_data);
  {
    std::unique_lock<std::mutex> lock(wait_info->mutex);
    // it is possible that all the events have already been signaled
    if (wait_info->event_count) {
      // GCOVR_EXCL_START non-deterministically executed
      wait_info->condition_variable.wait(
          lock, [&wait_info] { return wait_info->event_count == 0; });
      // GCOVR_EXCL_STOP
    }
  }
  wait_info->allocator->destroy(wait_info);
}
}  // namespace vk
