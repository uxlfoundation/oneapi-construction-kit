// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief host implementation of the mux_fence_s object.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef MUX_HOST_FENCE_H_INCLUDED
#define MUX_HOST_FENCE_H_INCLUDED
#include <mux/mux.h>

#include <atomic>

namespace host {
struct fence_s : mux_fence_s {
  fence_s(mux_device_t device);
  /// @see muxResetFence
  void reset();

  /// @see tryWait
  mux_result_t tryWait(uint64_t timeout);

  std::atomic<bool> thread_pool_signal;
  mux_result_t result;
};
}  // namespace host
#endif  // MUX_HOST_FENCE_H_INCLUDED
