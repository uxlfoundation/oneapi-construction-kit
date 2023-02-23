// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief riscv's fence interface.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef RISCV_FENCE_H_INCLUDED
#define RISCV_FENCE_H_INCLUDED

#include "mux/hal/fence.h"

namespace riscv {
/// @addtogroup riscv
/// @{

using fence_s = mux::hal::fence;

/// @}
}  // namespace riscv

#endif  // RISCV_FENCE_H_INCLUDED
