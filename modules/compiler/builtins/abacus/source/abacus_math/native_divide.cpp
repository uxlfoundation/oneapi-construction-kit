// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>

namespace {
template <typename T>
T native_divide(const T x, const T y) {
  return x / y;
}
}  // namespace

#define DEF(TYPE)                                          \
  TYPE ABACUS_API __abacus_native_divide(TYPE x, TYPE y) { \
    return native_divide<>(x, y);                          \
  }

DEF(abacus_float)
DEF(abacus_float2)
DEF(abacus_float3)
DEF(abacus_float4)
DEF(abacus_float8)
DEF(abacus_float16)
