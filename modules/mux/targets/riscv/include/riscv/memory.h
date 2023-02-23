// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief riscv's memory interface.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
