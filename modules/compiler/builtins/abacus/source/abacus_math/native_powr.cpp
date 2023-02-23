// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>

namespace {
template <typename T>
T native_powr(const T x, const T y) {
  // r = x ^ y
  // r = 2 ^ (log2(x ^ y))
  // r = 2 ^ (y * log2(x))
  return __abacus_native_exp2(y * __abacus_native_log2(x));
}
}  // namespace

abacus_float ABACUS_API __abacus_native_powr(abacus_float x, abacus_float y) {
  return native_powr<>(x, y);
}
abacus_float2 ABACUS_API __abacus_native_powr(abacus_float2 x,
                                              abacus_float2 y) {
  return native_powr<>(x, y);
}
abacus_float3 ABACUS_API __abacus_native_powr(abacus_float3 x,
                                              abacus_float3 y) {
  return native_powr<>(x, y);
}
abacus_float4 ABACUS_API __abacus_native_powr(abacus_float4 x,
                                              abacus_float4 y) {
  return native_powr<>(x, y);
}
abacus_float8 ABACUS_API __abacus_native_powr(abacus_float8 x,
                                              abacus_float8 y) {
  return native_powr<>(x, y);
}
abacus_float16 ABACUS_API __abacus_native_powr(abacus_float16 x,
                                               abacus_float16 y) {
  return native_powr<>(x, y);
}
