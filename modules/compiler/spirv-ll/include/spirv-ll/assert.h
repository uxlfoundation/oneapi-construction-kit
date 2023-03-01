// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef SPIRV_LL_SPV_ASSERT_INCLUDED_H
#define SPIRV_LL_SPV_ASSERT_INCLUDED_H

#if defined(_MSC_VER) && _MSC_VER < 1900
#define SPIRV_LL_FUNCTION __FUNCTION__
#else
#define SPIRV_LL_FUNCTION __func__
#endif

#ifndef NDEBUG
#include <cstdio>
#include <cstdlib>

#define SPIRV_LL_ABORT(MESSAGE)                                          \
  std::fprintf(stderr, "%s:%d: %s\n  %s\n", __FILE__, __LINE__, MESSAGE, \
               SPIRV_LL_FUNCTION);                                       \
  std::abort()

#define SPIRV_LL_ASSERT(CONDITION, MESSAGE) \
  if (!(CONDITION)) {                       \
    SPIRV_LL_ABORT(MESSAGE);                \
  }                                         \
  (void)0

#define SPIRV_LL_ASSERT_PTR(POINTER)     \
  if (nullptr == (POINTER)) {            \
    SPIRV_LL_ABORT(#POINTER " is null"); \
  }                                      \
  (void)0
#else
#define SPIRV_LL_ABORT(MESSAGE)
#define SPIRV_LL_ASSERT(CONDITION, MESSAGE)
#define SPIRV_LL_ASSERT_PTR(POINTER)
#endif

#endif  // SPIRV_LL_SPV_ASSERT_INCLUDED_H
