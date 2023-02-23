// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
/// riscv's semaphore interface.
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef RISCV_SEMAPHORE_H_INCLUDED
#define RISCV_SEMAPHORE_H_INCLUDED

#include "mux/hal/semaphore.h"

namespace riscv {
/// @addtogroup riscv
/// @{

using semaphore_s = mux::hal::semaphore;

/// @}
}  // namespace riscv

#endif  // RISCV_SEMAPHORE_H_INCLUDED
