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

#ifndef UR_EVENT_H_INCLUDED
#define UR_EVENT_H_INCLUDED

#include "cargo/expected.h"
#include "mux/mux.h"
#include "ur/base.h"

/// @brief Compute Mux specific implementation of the opaque
/// ur_event_handle_t_ API object.
struct ur_event_handle_t_ : ur::base {
  /// @brief constructor for creating event.
  ///
  /// @param[in] queue Queue to which this event belongs.
  /// @param[in] mux_fence Device->Host synchronization fence to wait on when
  /// waiting on the event.
  /// @param[in] mux_semaphore Device->device synchronization primitive used for
  /// event commands.
  ur_event_handle_t_(ur_queue_handle_t queue, mux_fence_t mux_fence,
                     mux_semaphore_t mux_semaphore)
      : queue{queue}, mux_fence{mux_fence}, mux_semaphore{mux_semaphore} {}
  ur_event_handle_t_(const ur_event_handle_t_ &) = delete;
  ur_event_handle_t_ &operator=(const ur_event_handle_t_ &) = delete;
  ~ur_event_handle_t_();

  /// @brief Factory method for creating events.
  /// @param[in] queue Queue to which this event belongs.
  ///
  /// @return Event object or error code if something went wrong.
  static cargo::expected<ur_event_handle_t, ur_result_t> create(
      ur_queue_handle_t queue);

  /// @brief Flush the queue associated with the event, waits for commands to
  /// finish
  ///
  /// @return Boolean indicating success of wait operation.
  /// @retval true wait was successful
  /// @retval false wait was failed
  bool wait();

  /// @brief Queue to which this event belongs.
  ur_queue_handle_t queue = nullptr;
  /// @brief Synchronization primitive used to do device->host synchronization
  /// when waiting on the event.
  mux_fence_t mux_fence = nullptr;
  /// @brief Synchronization primitive used to do device->device synchronization
  /// when waiting on the event (urEnqueueWaitEvents for instance).
  mux_semaphore_t mux_semaphore = nullptr;
};

#endif  // UR_EVENT_H_INCLUDED
