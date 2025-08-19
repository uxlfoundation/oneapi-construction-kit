// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

/// @file
/// Host's queue interface.

#ifndef HOST_QUEUE_H_INCLUDED
#define HOST_QUEUE_H_INCLUDED

#include <mux/mux.h>
#include <mux/utils/small_vector.h>

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace host {
/// @addtogroup host
/// @{

struct queue_s final : public mux_queue_s {
  /// @brief Construct the queue object.
  ///
  /// @param allocator Mux allocator to use.
  /// @param device Mux device which owns the queue.
  queue_s(mux_allocator_info_t allocator, mux_device_t device);

  /// @brief Destructor.
  ~queue_s();

  /// @brief Send the command group completed signal.
  ///
  /// @note This member function is **not** thread safe, callers must hold a
  /// lock on `mutex`.
  ///
  /// @param group The completed command group.
  /// @param terminate Should the queue tell the thread pool to terminate,
  /// `true` will terminate, `false` will not.
  void signalCompleted(mux_command_buffer_t group, bool terminate);

  /// @brief Add a command group to the queue.
  ///
  /// @note This member function is **not** thread safe, callers must hold a
  /// lock on `mutex`.
  ///
  /// @param group The command group to enqueue.
  /// @param fence The fence to signal when group completes.
  /// @param numWaits The number of wait semaphores to signal on completion.
  ///
  /// @return Returns `mux_error_success` or `mux_error_out_of_memory`.
  mux_result_t addGroup(mux_command_buffer_t group, mux_fence_t fence,
                        uint64_t numWaits);

  /// @brief Atomic counter of the current number of running command groups.
  std::atomic<uint32_t> runningGroups;

  /// @brief Mutex for users to lock to ensure ordering.
  std::mutex mutex;

  /// @brief Holds signaling information associated to a command buffer
  /// dispatch instance.
  struct signal_info_s {
    /// @brief How many times this dispatch instance needs to be signaled by
    /// wait semaphores before it can be executed.
    uint64_t wait_count;
    /// @brief A fence object to signal upon completion or termination of the
    /// command buffer dispatch instance.
    mux_fence_t fence;
  };
  /// @brief List of pairs of command group and wait count.
  /// BIG IMPORTANT WARNING:
  /// If one day we want to support simultaneous use then we could end up
  /// with two copies of the same command  buffer in the vector paired with
  /// different fences. When it comes to signalling a command buffer we
  /// currently have no way of knowing which command buffer to signal. Bottom
  /// line, if we want simultaneous use, we'll need to change how we hold and
  /// signal fences.
  mux::small_vector<std::pair<mux_command_buffer_t, signal_info_s>, 8>
      signalInfos;
};

/// @}
}  // namespace host

#endif  // HOST_QUEUE_H_INCLUDED
