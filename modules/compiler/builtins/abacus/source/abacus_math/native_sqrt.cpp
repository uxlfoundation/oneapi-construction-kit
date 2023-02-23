// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T>
T native_sqrt(const T x) {
  return x * __abacus_native_rsqrt(x);
}
}  // namespace

abacus_float ABACUS_API __abacus_native_sqrt(abacus_float x) {
  return native_sqrt<>(x);
}
abacus_float2 ABACUS_API __abacus_native_sqrt(abacus_float2 x) {
  return native_sqrt<>(x);
}
abacus_float3 ABACUS_API __abacus_native_sqrt(abacus_float3 x) {
  return native_sqrt<>(x);
}
abacus_float4 ABACUS_API __abacus_native_sqrt(abacus_float4 x) {
  return native_sqrt<>(x);
}
abacus_float8 ABACUS_API __abacus_native_sqrt(abacus_float8 x) {
  return native_sqrt<>(x);
}
abacus_float16 ABACUS_API __abacus_native_sqrt(abacus_float16 x) {
  return native_sqrt<>(x);
}
