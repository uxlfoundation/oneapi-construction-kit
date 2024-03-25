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

#include <mux/hal/fence.h>

#include <cstdint>
#include <mutex>

#include "mux/mux.h"

namespace mux {
namespace hal {
fence::fence(mux_device_t device) { this->device = device; }

void fence::signal(mux_result_t result) {
  const std::unique_lock<std::mutex> lock(mutex);
  completed = true;
  this->result = result;
  condition_variable.notify_all();
}

void fence::reset() {
  const std::lock_guard<std::mutex> lock(mutex);
  completed = false;
  result = mux_fence_not_ready;
}

mux_result_t fence::tryWait(uint64_t timeout) {
  std::unique_lock<std::mutex> lock(mutex);

  // If timeout is UINT64_MAX, we need to wait on fence instead of timeout
  // value.
  if (timeout == UINT64_MAX) {
    condition_variable.wait(lock, [this] { return completed; });
    return mux_success;
  } else if ((mux_fence_not_ready == result) && timeout) {
    // If the fence isn't already signaled and there is a timeout then we need
    // to try and wait.
    // Clamp timeout to roughly one year (3e+16) in nanoseconds so we don't
    // overflow chrono::steady_clock in the wait_for call.
    timeout = std::min(timeout, static_cast<uint64_t>(0x6A94D74F430000));
    auto duration = std::chrono::duration<uint64_t, std::nano>(timeout);
    condition_variable.wait_for(lock, duration);
  }

  // Otherwise we just query the result directly.
  switch (result) {
    case mux_success:
    case mux_fence_not_ready:
      return result;
    default:
      return mux_error_fence_failure;
  }
}
}  // namespace hal
}  // namespace mux
