// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
/// riscv's hal interface.
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef RISCV_HAL_H_INCLUDED
#define RISCV_HAL_H_INCLUDED

#include <hal.h>
#include <hal_library.h>

namespace riscv {

/// @brief Load the hal library if needed and return an hal_t instance.
///
///  returns nullptr on failure.
hal::hal_t *hal_get();

/// @brief Unload the hal library.
void hal_unload();

}  // namespace riscv

#endif  // RISCV_HAL_H_INCLUDED
