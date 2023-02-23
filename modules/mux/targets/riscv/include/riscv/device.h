// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
/// Riscv's device interface.
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
