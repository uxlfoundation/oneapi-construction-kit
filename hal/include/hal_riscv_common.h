// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Device Hardware Abstraction Layer common routines.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef RISCV_HAL_COMMON_H_INCLUDED
#define RISCV_HAL_COMMON_H_INCLUDED

#include <assert.h>
#include <hal.h>
#include <hal_riscv.h>
#include <hal_types.h>

namespace riscv {
/// @addtogroup riscv
/// @{

bool update_info_from_riscv_isa_description(
    const char *str, ::hal::hal_device_info_t &info,
    hal_device_info_riscv_t &riscv_info);
}  // namespace riscv

#endif
