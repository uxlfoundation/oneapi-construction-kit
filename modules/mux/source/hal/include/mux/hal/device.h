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
/// @brief HAL base implementation of the mux_device_s object.

#ifndef MUX_HAL_DEVICE_H_INCLUDED
#define MUX_HAL_DEVICE_H_INCLUDED

#include "hal_profiler.h"
#include "mux/mux.h"
#include "mux/utils/allocator.h"

namespace mux {
namespace hal {
struct device : mux_device_s {
  explicit device(mux_device_info_t info);

  /// @brief Hardware abstraction layer for device access.
  ::hal::hal_t *hal;

  /// @brief Hardware abstraction layer device instance.
  ::hal::hal_device_t *hal_device;

  /// @brief HAL profiler
  ::hal::util::hal_profiler_t profiler;
};
} // namespace hal
} // namespace mux

#endif // MUX_HAL_DEVICE_H_INCLUDED
