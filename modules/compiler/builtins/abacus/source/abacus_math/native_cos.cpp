// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>

namespace {
// only valid in the range [-pi .. pi]
template <typename T>
T native_cos(const T x) {
  const T xAbs = __abacus_fabs(x);

  const T bit = (-xAbs + ABACUS_PI_2_F) * ABACUS_1_PI_F;

  return (bit - bit * __abacus_fabs(bit)) * 4.0f;
}
}  // namespace

abacus_float ABACUS_API __abacus_native_cos(abacus_float x) {
  return native_cos<>(x);
}
abacus_float2 ABACUS_API __abacus_native_cos(abacus_float2 x) {
  return native_cos<>(x);
}
abacus_float3 ABACUS_API __abacus_native_cos(abacus_float3 x) {
  return native_cos<>(x);
}
abacus_float4 ABACUS_API __abacus_native_cos(abacus_float4 x) {
  return native_cos<>(x);
}
abacus_float8 ABACUS_API __abacus_native_cos(abacus_float8 x) {
  return native_cos<>(x);
}
abacus_float16 ABACUS_API __abacus_native_cos(abacus_float16 x) {
  return native_cos<>(x);
}
