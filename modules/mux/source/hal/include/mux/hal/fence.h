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
/// @brief HAL base implementation of the mux_fence_s object.

#ifndef MUX_HAL_FENCE_H_INCLUDED
#define MUX_HAL_FENCE_H_INCLUDED

#include <condition_variable>
#include <type_traits>

#include "cargo/expected.h"
#include "mux/mux.h"
#include "mux/utils/allocator.h"
namespace mux {
namespace hal {
struct fence : mux_fence_s {
  fence(mux_device_t device);

  /// @see muxCreateFence
  template <class Fence>
  static cargo::expected<Fence *, mux_result_t> create(
      mux_device_t device, mux::allocator allocator) {
    static_assert(std::is_base_of_v<mux::hal::fence, Fence>,
                  "template type Fence must derive from mux::hal::fence");
    const auto fence = allocator.create<Fence>(device);
    if (!fence) {
      return cargo::make_unexpected(mux_error_out_of_memory);
    }
    return fence;
  }

  /// @see muxDestroyFence
  template <class Fence>
  static void destroy(mux_device_t device, Fence *fence,
                      mux::allocator allocator) {
    static_assert(std::is_base_of_v<mux::hal::fence, Fence>,
                  "template type Fence must derive from mux::hal::fence");
    (void)device;
    allocator.destroy(fence);
  }

  /// @see muxResetFence
  void reset();

  /// @see tryWait
  mux_result_t tryWait(uint64_t timeout);

  void signal(mux_result_t result);

 private:
  std::mutex mutex;
  std::condition_variable condition_variable;
  mux_result_t result = mux_fence_not_ready;
  bool completed = false;
};
}  // namespace hal
}  // namespace mux
#endif  // MUX_HAL_FENCE_H_INCLUDED
