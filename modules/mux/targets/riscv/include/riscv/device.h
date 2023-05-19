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
/// Riscv's device interface.

#ifndef RISCV_DEVICE_H_INCLUDED
#define RISCV_DEVICE_H_INCLUDED

#include "mux/hal/device.h"
#include "riscv/queue.h"
#include "riscv/riscv.h"

namespace riscv {
/// @addtogroup riscv
/// @{
struct device_s final : mux::hal::device {
  /// @brief Main constructor.
  ///
  /// @param info The device info associated with this device.
  /// @param allocator The mux allocate to use for allocations.
  explicit device_s(mux_device_info_t info, mux::allocator allocator)
      : mux::hal::device(info), queue(allocator, this) {}

  /// @brief Riscv's single queue for command execution.
  riscv::queue_s queue;
};
/// @}
};      // namespace riscv
#endif  // RISCV_DEVICE_H_INCLUDED
