// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <mux/hal/fence.h>

#include <cstdint>
#include <mutex>

#include "mux/mux.h"

namespace mux {
namespace hal {
fence::fence(mux_device_t device) { this->device = device; };

void fence::signal(mux_result_t result) {
  std::unique_lock<std::mutex> lock(mutex);
  completed = true;
  this->result = result;
  condition_variable.notify_all();
}

void fence::reset() {
  std::lock_guard<std::mutex> lock(mutex);
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
