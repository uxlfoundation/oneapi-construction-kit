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
/// @brief HAL base implementation of the mux_executable_s object.

#ifndef MUX_HAL_EXECUTABLE_H_INCLUDED
#define MUX_HAL_EXECUTABLE_H_INCLUDED

#include <algorithm>

#include "cargo/expected.h"
#include "mux/hal/device.h"
#include "mux/mux.h"
#include "mux/utils/allocator.h"
#include "mux/utils/dynamic_array.h"

namespace mux {
namespace hal {
struct executable : mux_executable_s {
  executable(mux::hal::device *device,
             mux::dynamic_array<uint8_t> &&object_code);

  template <class Executable>
  static cargo::expected<Executable *, mux_result_t> create(
      mux::hal::device *device, const void *binary, uint64_t binary_length,
      mux::allocator allocator) {
    static_assert(
        std::is_base_of_v<mux::hal::executable, Executable>,
        "template type Executable must derive from mux::hal::executable");
    mux::dynamic_array<uint8_t> object_code{allocator};
    if (object_code.alloc(binary_length)) {
      return cargo::make_unexpected(mux_error_out_of_memory);
    }
    std::copy_n(reinterpret_cast<const uint8_t *>(binary),
                static_cast<size_t>(binary_length), object_code.data());
    auto executable =
        allocator.create<Executable>(device, std::move(object_code));
    if (!executable) {
      return cargo::make_unexpected(mux_error_out_of_memory);
    }
    return executable;
  }

  template <class Executable>
  static void destroy(mux::hal::device *device,
                      mux::hal::executable *executable,
                      mux::allocator allocator) {
    static_assert(
        std::is_base_of_v<mux::hal::executable, Executable>,
        "template type Executable must derive from mux::hal::executable");
    (void)device;
    allocator.destroy(executable);
  }

  /// @brief Compiled object code.
  ///
  /// This comes either from LLVM (if the executable is created from source),
  /// or an ELF file (if the executable is created from a pre-compiled binary).
  /// It is not used with built-in kernels.
  mux::dynamic_array<uint8_t> object_code;
};
}  // namespace hal
}  // namespace mux

#endif  // MUX_HAL_EXECUTABLE_H_INCLUDED
