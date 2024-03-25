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
/// @brief HAL base implementation of the mux_semaphore_s object.

#ifndef MUX_HAL_SEMAPHORE_H_INCLUDED
#define MUX_HAL_SEMAPHORE_H_INCLUDED

#include <atomic>
#include <type_traits>

#include "cargo/expected.h"
#include "hal.h"
#include "mux/mux.h"
#include "mux/utils/allocator.h"
#include "mux/utils/small_vector.h"

namespace mux {
namespace hal {
struct semaphore : mux_semaphore_s {
  semaphore(mux_device_t device);

  /// @see muxCreateSemaphore
  template <class Semaphore>
  static cargo::expected<Semaphore *, mux_result_t> create(
      mux_device_t device, mux::allocator allocator) {
    static_assert(
        std::is_base_of_v<mux::hal::semaphore, Semaphore>,
        "template type Semaphore must derive from mux::hal::semaphore");
    const auto semaphore = allocator.create<Semaphore>(device);
    if (!semaphore) {
      return cargo::make_unexpected(mux_error_out_of_memory);
    }
    return semaphore;
  }

  /// @see muxDestroySemaphore
  template <class Semaphore>
  static void destroy(mux_device_t device, Semaphore *semaphore,
                      mux::allocator allocator) {
    static_assert(
        std::is_base_of_v<mux::hal::semaphore, Semaphore>,
        "template type Semaphore must derive from mux::hal::semaphore");
    (void)device;
    allocator.destroy(semaphore);
  }

  /// @see muxResetSemaphore
  void reset();

  void signal();
  bool is_signalled();

  void terminate();
  bool is_terminated();

 private:
  enum states : uint32_t {
    SIGNAL = 0x1,
    TERMINATE = 0x80000000,
  };
  std::atomic_uint32_t status;
};
}  // namespace hal
}  // namespace mux

#endif  // MUX_HAL_SEMAPHORE_H_INCLUDED
