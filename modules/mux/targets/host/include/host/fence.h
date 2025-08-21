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
///
/// @brief host implementation of the mux_fence_s object.

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
} // namespace host
#endif // MUX_HOST_FENCE_H_INCLUDED
