// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Print a backtrace of the call stack.
///
/// @copyright
/// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef DEBUG_BACKTRACE_H_INCLUDED
#define DEBUG_BACKTRACE_H_INCLUDED

#ifndef DEBUG_BACKTRACE_ENABLED
#error debug/backtrace.h can not be included if CA_ENABLE_DEBUG_BACKTRACE=OFF
#endif

#include <cstdio>

/// @brief Print a backtrace of the call stack with file, line info.
#define DEBUG_BACKTRACE                                                 \
  {                                                                     \
    std::fprintf(stderr, "backtrace from %s:%d\n", __FILE__, __LINE__); \
    debug::print_backtrace(stderr);                                     \
  }

namespace debug {
/// @brief Print a backtrace of the current frame to a file.
///
/// @param out Output destination for the backtrace.
void print_backtrace(FILE *out);
}  // namespace debug

#endif  // DEBUG_BACKTRACE_H_INCLUDED
