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
#include <host/device.h>
#include <host/fence.h>

#include "host/host.h"
#include "mux/mux.h"

namespace host {

fence_s::fence_s(mux_device_t device)
    : thread_pool_signal{false}, result{mux_error_internal} {
  this->device = device;
}

void fence_s::reset() {
  thread_pool_signal = false;
  result = mux_fence_not_ready;
}

mux_result_t fence_s::tryWait(uint64_t timeout) {
  // If timeout is UINT64_MAX, we need to wait on fence instead of timeout
  // value.
  if (timeout == UINT64_MAX) {
    auto *hostDevice = static_cast<host::device_s *>(device);
    hostDevice->thread_pool.wait(&thread_pool_signal);
    assert((result != mux_error_internal) &&
           "Thread pool was signalled yet no fence result was set");
    return result;
  }

  // See if the fence is already signaled.
  auto result = thread_pool_signal ? this->result : mux_fence_not_ready;
  assert((result != mux_error_internal) &&
         "Thread pool was signalled yet no fence result was set");

  // If the fence hasn't been signaled go to sleep for the requested timeout
  // period (if it is exists) periodically waking up every nanosecond to check
  // the result.
  uint64_t time_waited = 0;
  constexpr uint64_t increment = 1;
  while (time_waited < timeout) {
    std::this_thread::sleep_for(
        std::chrono::duration<uint64_t, std::nano>(increment));

    // Check the result again and if it is done we can exit.
    result = thread_pool_signal ? this->result : mux_fence_not_ready;
    assert((result != mux_error_internal) &&
           "Thread pool was signalled yet no fence result was set");
    if (mux_success == result) {
      return mux_success;
    }

    time_waited += increment;
  }

  // Check one last time.
  result = thread_pool_signal ? this->result : mux_fence_not_ready;
  assert((result != mux_error_internal) &&
         "Thread pool was signalled yet no fence result was set");
  return result;
}
}  // namespace host

mux_result_t hostCreateFence(mux_device_t device,
                             mux_allocator_info_t allocator_info,
                             mux_fence_t *out_fence) {
  mux::allocator allocator(allocator_info);
  auto *fence = allocator.create<host::fence_s>(device);
  if (!fence) {
    return mux_error_out_of_memory;
  }
  *out_fence = fence;

  return mux_success;
}

void hostDestroyFence(mux_device_t device, mux_fence_t fence,
                      mux_allocator_info_t allocator_info) {
  (void)device;
  mux::allocator allocator(allocator_info);
  allocator.destroy(static_cast<host::fence_s *>(fence));
}
