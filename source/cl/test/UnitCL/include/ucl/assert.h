// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef UCL_ASSERT_H_INCLUDED
#define UCL_ASSERT_H_INCLUDED

#include <cstdio>
#include <cstdlib>

#define UCL_ASSERT(CONDITION, MESSAGE)                                 \
  if (!(CONDITION)) {                                                  \
    std::fprintf(stderr, "%s: %d: %s\n", __FILE__, __LINE__, MESSAGE); \
    std::abort();                                                      \
  }

#define UCL_ABORT(FORMAT, ...)                                              \
  {                                                                         \
    std::fprintf(stderr, "abort: %s: %d: " FORMAT "\n", __FILE__, __LINE__, \
                 __VA_ARGS__);                                              \
    std::abort();                                                           \
  }

#endif  // UCL_ASSERT_H_INCLUDED
