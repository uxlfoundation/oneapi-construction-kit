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
/// @brief riscv's memory interface.

#ifndef RISCV_MEMORY_H_INCLUDED
#define RISCV_MEMORY_H_INCLUDED

#include "mux/hal/memory.h"
#include "riscv/device.h"

namespace riscv {
/// @addtogroup riscv
/// @{

struct memory_s : mux::hal::memory {
  memory_s(uint64_t size, uint32_t properties, ::hal::hal_addr_t data,
           void *origHostPtr)
      : mux::hal::memory(size, properties, data, origHostPtr) {}

  /// Extend mux::hal::memory::flushToDevice() to also update profiler counters
  /// once flushing is completed.
  ///
  /// @see mux::hal::memory::flushToDevice()
  mux_result_t flushToDevice(riscv::device_s *device, uint64_t offset,
                             uint64_t size);

  /// Extend mux::hal::memory::flushFromDevice() to also update profiler
  /// counters once flushing is completed.
  ///
  /// @see mux::hal::memory::flushFromDevice()
  mux_result_t flushFromDevice(riscv::device_s *device, uint64_t offset,
                               uint64_t size);
};

/// @}
}  // namespace riscv

#endif  // RISCV_MEMORY_H_INCLUDED
