// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>

namespace {
// only valid in the range [-pi .. pi]
template <typename T>
T native_tan(const T x) {
  return __abacus_native_sin(x) / __abacus_native_cos(x);
}
}  // namespace

abacus_float ABACUS_API __abacus_native_tan(abacus_float x) {
  return native_tan<>(x);
}
abacus_float2 ABACUS_API __abacus_native_tan(abacus_float2 x) {
  return native_tan<>(x);
}
abacus_float3 ABACUS_API __abacus_native_tan(abacus_float3 x) {
  return native_tan<>(x);
}
abacus_float4 ABACUS_API __abacus_native_tan(abacus_float4 x) {
  return native_tan<>(x);
}
abacus_float8 ABACUS_API __abacus_native_tan(abacus_float8 x) {
  return native_tan<>(x);
}
abacus_float16 ABACUS_API __abacus_native_tan(abacus_float16 x) {
  return native_tan<>(x);
}
