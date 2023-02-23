// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Compiler specified limits.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_LIMITS_H_INCLUDED
#define COMPILER_LIMITS_H_INCLUDED

#include <cstddef>
#include <cstdint>

namespace compiler {
/// @addtogroup compiler
/// @{

enum : size_t {
  PRINTF_BUFFER_SIZE = 1024 * 1024,  ///< 1MiB is spec mandated minimum.
};

/// @}
}  // namespace compiler

#endif  // COMPILER_LIMITS_H_INCLUDED
