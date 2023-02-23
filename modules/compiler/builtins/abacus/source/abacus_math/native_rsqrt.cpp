// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T>
T native_rsqrt(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  const SignedType i =
      (SignedType)0x5f375a86 - (abacus::detail::cast::as<SignedType>(x) >> 1);
  const T y = abacus::detail::cast::as<T>(i);
  return y * ((T)1.5f - (x * y * y * 0.5f));
}
}  // namespace

abacus_float ABACUS_API __abacus_native_rsqrt(abacus_float x) {
  return native_rsqrt<>(x);
}
abacus_float2 ABACUS_API __abacus_native_rsqrt(abacus_float2 x) {
  return native_rsqrt<>(x);
}
abacus_float3 ABACUS_API __abacus_native_rsqrt(abacus_float3 x) {
  return native_rsqrt<>(x);
}
abacus_float4 ABACUS_API __abacus_native_rsqrt(abacus_float4 x) {
  return native_rsqrt<>(x);
}
abacus_float8 ABACUS_API __abacus_native_rsqrt(abacus_float8 x) {
  return native_rsqrt<>(x);
}
abacus_float16 ABACUS_API __abacus_native_rsqrt(abacus_float16 x) {
  return native_rsqrt<>(x);
}
