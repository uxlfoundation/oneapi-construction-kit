// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Core's system interface.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef UTILS_SYSTEM_H_INCLUDED
#define UTILS_SYSTEM_H_INCLUDED

#include <cstdint>

/// @addtogroup utils
/// @{

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || \
    (defined(__riscv) && __riscv_xlen == 64)
#define UTILS_SYSTEM_64_BIT 1
#else
#define UTILS_SYSTEM_32_BIT 1
#endif

#if defined(__arm__) || defined(__thumb__) || defined(_M_ARM) || \
    defined(_M_ARMT) || defined(__aarch64__)
#define UTILS_SYSTEM_ARM 1
#elif defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) || \
    defined(_M_X64)
#define UTILS_SYSTEM_X86 1
#elif defined(__riscv) || defined(__riscv__)
#define UTILS_SYSTEM_RISCV 1
#else
#error Unknown host system being compiled for!
#endif

/// @}

namespace utils {
/// @addtogroup utils
/// @{

/// @brief Get the current system clock tick count in nanoseconds.
///
/// @return Clock time count measured in nanoseconds.
uint64_t timestampMicroSeconds();

/// @brief Get the current system clock tick count in nanoseconds.
///
/// @return Clock time count measured in nanoseconds.
uint64_t timestampNanoSeconds();

/// @}
}  // utils

#endif  // UTILS_SYSTEM_H_INCLUDED
