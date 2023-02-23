// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

#include <abacus/internal/is_denorm.h>
#include <abacus/internal/math_defines.h>
#include <abacus/internal/rsqrt_unsafe.h>

namespace {
template <typename T>
T half_rsqrt(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;

  const T recipX = (T)1.0f / x;

  const UnsignedType xUint = abacus::detail::cast::as<UnsignedType>(x);

  // See rsqrt.cpp for comments documenting this limit and the scaling
  // in processedX.
  const SignedType xBig = (xUint >= 0x7e6eb3c0);

  const SignedType xSmall = abacus::internal::is_denorm(x);

  // xUint | F_HIDDEN_BIT  sets the exponent to -126
  // 16777216              2^24
  // Multiplication        exponent = -126 + 24 = -102
  //
  // 0x0C800000            2^(-102)
  //
  // processedX            (x * 2^24) + 2^-102 - 2^-102
  //                       (x * 2^24)
  T processedX = __abacus_select(
      x,
      abacus::detail::cast::as<T>(xUint | F_HIDDEN_BIT) * 16777216.0f -
          __abacus_as_float(0x0C800000),
      xSmall);

  processedX = __abacus_select(processedX, x * 0.0625f, xBig);

  // 1/sqrt(x)
  T ans = abacus::internal::rsqrt_unsafe(processedX, 2);

  ans = __abacus_select(ans, ans * 4096.0f, xSmall);

  ans = __abacus_select(ans, ans * 0.25f, xBig);

  ans = __abacus_select(ans, recipX, __abacus_isinf(x));
  ans = __abacus_select(ans, ABACUS_NAN, __abacus_signbit(x));
  ans = __abacus_select(ans, recipX, (__abacus_fabs(x) == 0.0f) & ~xSmall);

  return ans;
}
}  // namespace

abacus_float ABACUS_API __abacus_half_rsqrt(abacus_float x) {
  return half_rsqrt<>(x);
}
abacus_float2 ABACUS_API __abacus_half_rsqrt(abacus_float2 x) {
  return half_rsqrt<>(x);
}
abacus_float3 ABACUS_API __abacus_half_rsqrt(abacus_float3 x) {
  return half_rsqrt<>(x);
}
abacus_float4 ABACUS_API __abacus_half_rsqrt(abacus_float4 x) {
  return half_rsqrt<>(x);
}
abacus_float8 ABACUS_API __abacus_half_rsqrt(abacus_float8 x) {
  return half_rsqrt<>(x);
}
abacus_float16 ABACUS_API __abacus_half_rsqrt(abacus_float16 x) {
  return half_rsqrt<>(x);
}
