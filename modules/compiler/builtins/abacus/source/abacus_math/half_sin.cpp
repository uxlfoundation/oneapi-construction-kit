// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>

#include <abacus/internal/half_range_reduction.h>
#include <abacus/internal/half_sincos_approx.h>

namespace {
template <typename T>
T half_sin(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  SignedType octet;
  T xReduced = abacus::internal::half_range_reduction(x, &octet);

  T cosApprox;
  T sinApprox = abacus::internal::half_sincos_approx(xReduced, &cosApprox);

  octet = octet & 0x7;

  sinApprox *= __abacus_select((T)1.0f, (T)-1.0f,
                               (octet == 3) | (octet == 4) | (octet == 7));

  T ansPart = __abacus_select(sinApprox, cosApprox, (octet & 0x3) != 0);

  T ans = __abacus_select(ansPart, (cosApprox + sinApprox) * ABACUS_SQRT1_2_F,
                          (octet & 0x1) != 0);

  ans *= __abacus_select((T)-1.0f, (T)1.0f, (octet < 5));

  // Note this is not required by the spec but bruteforce expects a
  // value in the range [-1 .. 1] for out of range values.
  ans = __abacus_select(ans, 0.0f, __abacus_fabs(x) > 65536.0f);

  return __abacus_select(ABACUS_NAN, ans, __abacus_isfinite(x));
}
}  // namespace

abacus_float ABACUS_API __abacus_half_sin(abacus_float x) {
  return half_sin<>(x);
}
abacus_float2 ABACUS_API __abacus_half_sin(abacus_float2 x) {
  return half_sin<>(x);
}
abacus_float3 ABACUS_API __abacus_half_sin(abacus_float3 x) {
  return half_sin<>(x);
}
abacus_float4 ABACUS_API __abacus_half_sin(abacus_float4 x) {
  return half_sin<>(x);
}
abacus_float8 ABACUS_API __abacus_half_sin(abacus_float8 x) {
  return half_sin<>(x);
}
abacus_float16 ABACUS_API __abacus_half_sin(abacus_float16 x) {
  return half_sin<>(x);
}
