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

/// @file
///
/// @brief

#ifndef UR_QUEUE_H_INCLUDED
#define UR_QUEUE_H_INCLUDED

#include <deque>
#include <mutex>

#include "cargo/expected.h"
#include "cargo/ring_buffer.h"
#include "cargo/small_vector.h"
#include "mux/mux.hpp"
#include "ur/base.h"
#include "ur/context.h"

/// @brief Compute Mux specific implementation of the opaque
/// ur_queue_handle_t_ API object.
struct ur_queue_handle_t_ : ur::base {
  /// @brief Constructor for creating queue.
  ///
  /// @param[in] context Context to which the queue should belong.
  /// @param[in] device Device which the queue should target.
  /// @param[in] props Properties to create the queue with.
  /// @param[in] mux_queue Underlying mux queue used to enqueue commands to the
  /// target.
  ur_queue_handle_t_(ur_context_handle_t context, ur_device_handle_t device,
                     ur_queue_flags_t props, mux_queue_t mux_queue)
      : context{context}, device{device}, props{props}, mux_queue{mux_queue} {}
  ur_queue_handle_t_(const ur_queue_handle_t_ &) = delete;
  ur_queue_handle_t_ &operator=(const ur_queue_handle_t_ &) = delete;

  /// @brief Destructor needed for cleaning up command buffers and event
  /// objects that could still be associated with the queue.
  ~ur_queue_handle_t_();

  /// @brief Factory method for creating queues.
  ///
  /// @param[in] hContext Context to which this queue will belong.
  /// @param[in] hDevice Device which this queue will target.
  /// @param[in] props Properties to create the queue with.
  ///
  /// @return The newly created queue or an error if something went wrong.
  static cargo::expected<ur_queue_handle_t, ur_result_t> create(
      ur_context_handle_t hContext, ur_device_handle_t hDevice,
      ur_queue_flags_t props);

  /// @brief Flush all pending work in the queue to the device for execution.
  ///
  /// @return UR_RESULT_SUCCESS or a UR error code on failure.
  ur_result_t flush();

  /// @brief Flush the queue and wait for all work to complete.
  ///
  /// @retval UR_RESULT_SUCCESS or a UR error code on failure.
  ur_result_t wait();

  /// @brief Retrieve the unique index of the device associated to the queue
  /// in the context.
  ///
  /// @return The unique index of the device for the queue in the context.
  uint32_t getDeviceIdx() const { return context->getDeviceIdx(device); }

  /// @brief The context to which this queue belongs.
  ur_context_handle_t context = nullptr;
  /// @brief The device this queue targets.
  ur_device_handle_t device = nullptr;
  /// @brief The properties this queue was created with.
  ur_queue_flags_t props = 0;
  /// @brief The underlying mux queue used to enqueue command on the target.
  mux_queue_t mux_queue = nullptr;

  /// @brief State required for tracking a command buffer while it's in use.
  struct dispatch_state_t {
    /// @brief The command buffer being dispatched.
    mux_command_buffer_t command_buffer;
    /// @brief List of semaphores this dispatch must wait for.
    cargo::small_vector<mux_semaphore_t, 8> wait_semaphores;
    /// @brief Event the dispatch will signal on completion, encompasses a mux
    /// semaphore and a mux fence.
    ur_event_handle_t signal_event;
  };

  /// @brief Get a mux command buffer and add it to the queue for dispatch.
  ///
  /// @note This member function is not thread-safe, callers **must** hold a
  /// lock on `mutex` when calling it.
  ///
  /// @param signal_event Event object the command buffer will signal when
  /// complete.
  /// @param num_wait_events Length of `wait_events`.
  /// @param wait_events Optional list of events for the command buffer to wait
  /// for before executing.
  ///
  /// @return Returns the expected command buffer or `CL_OUT_OF_RESOURCES`.
  [[nodiscard]] cargo::expected<mux_command_buffer_t, ur_result_t>
  getCommandBuffer(const ur_event_handle_t signal_event,
                   uint32_t num_wait_events,
                   const ur_event_handle_t *wait_events);

  /// @brief Resets the given mux command buffer and then returns it to the
  /// cache if there's room, or destroys it if there isn't.
  void destroyCommandBuffer(mux_command_buffer_t command_buffer);

  /// @brief Removes any completed dispatches from `running_dispatches` and
  /// cleans up the various objects associated with them.
  ///
  /// @return UR_RESULT_SUCCESS or a UR error code on failure.
  ur_result_t cleanupCompletedCommandBuffers();

  /// @brief Dispatches that haven't been flushed to the device for execution.
  cargo::small_vector<dispatch_state_t, 16> pending_dispatches;

  /// @brief Double ended queue to track currently running dispatches.
  std::deque<dispatch_state_t> running_dispatches;

  /// @brief A set of command buffers that are idle and ready to use.
  cargo::ring_buffer<mux_command_buffer_t, 16> cached_command_buffers;

  /// @brief List of completed events which are still being waited on by running
  /// dispatches.
  cargo::small_vector<ur_event_handle_t, 32> completed_events;

  /// @brief Mutex to lock when creating command buffers, flushing pending
  /// dispatches, or otherwise accessing data members of this object.
  std::mutex mutex;
};

#endif  // UR_QUEUE_H_INCLUDED
