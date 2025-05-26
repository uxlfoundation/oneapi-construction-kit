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

#include <host/device.h>
#include <host/queue.h>
#include <host/semaphore.h>
#include <mux/utils/allocator.h>

namespace host {
semaphore_s::semaphore_s(mux_device_t device,
                         mux_allocator_info_t allocator_info)
    : signalled(false), failed(false), waitingGroups(allocator_info) {
  this->device = device;
}

void semaphore_s::signal(bool terminate) {
  // Set the signalled state to true.
  signalled = true;
  failed = terminate;

  // This is only called from hostDispatch which already holds a lock on the
  // queues mutex.
  auto &hostQueue = static_cast<host::device_s *>(device)->queue;

  // Run through our waits to signal them.
  for (size_t i = 0; i < waitingGroups.size(); i++) {
    auto command_buffer = waitingGroups[i];
    hostQueue.signalCompleted(command_buffer, terminate);
  }
}

mux_result_t semaphore_s::addWait(mux_command_buffer_t group) {
  // Check if the semaphore has already been signalled.
  if (signalled) {
    // This is only called from hostDispatch which already holds a lock on the
    // queues mutex.
    static_cast<host::device_s *>(this->device)
        ->queue.signalCompleted(group, failed);
  } else {
    // and save the queue and group onto the list
    if (cargo::success != waitingGroups.push_back(group)) {
      return mux_error_out_of_memory;
    }
  }

  return mux_success;
}

void semaphore_s::reset() {
  signalled = false;
  failed = false;
  waitingGroups.clear();
}

semaphore_s::~semaphore_s() {}
}  // namespace host

mux_result_t hostCreateSemaphore(mux_device_t device,
                                 mux_allocator_info_t allocator_info,
                                 mux_semaphore_t *out_semaphore) {
  mux::allocator allocator(allocator_info);

  auto semaphore = allocator.create<host::semaphore_s>(device, allocator_info);
  if (nullptr == semaphore) {
    return mux_error_out_of_memory;
  }

  *out_semaphore = semaphore;

  return mux_success;
}

void hostDestroySemaphore(mux_device_t device, mux_semaphore_t semaphore,
                          mux_allocator_info_t allocator_info) {
  (void)device;
  mux::allocator allocator(allocator_info);
  allocator.destroy(static_cast<host::semaphore_s *>(semaphore));
}
