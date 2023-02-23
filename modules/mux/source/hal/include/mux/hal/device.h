// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief HAL base implementation of the mux_device_s object.
///
/// @copyright
/// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
}  // namespace hal
}  // namespace mux

#endif  // MUX_HAL_DEVICE_H_INCLUDED
