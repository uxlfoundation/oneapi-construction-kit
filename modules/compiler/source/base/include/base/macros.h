// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Utility macros used to implement the compiler library.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_BASE_MACROS_H_INCLUDED
#define COMPILER_BASE_MACROS_H_INCLUDED

#include <cstdio>
#include <cstdlib>

/// @brief Display a message to stderr and abort.
///
/// @param MESSAGE Message to display prior to aborting.
#define CPL_ABORT(MESSAGE)                                                  \
  do {                                                                      \
    (void)std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, MESSAGE); \
    std::abort();                                                           \
  } while (0)

#endif  // COMPILER_BASE_MACROS_H_INCLUDED
