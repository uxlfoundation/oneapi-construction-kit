// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_type_traits.h>

#include <abacus/internal/floor_unsafe.h>
#include <abacus/internal/horner_polynomial.h>
#include <abacus/internal/ldexp_unsafe.h>

namespace {
template <typename T>
T native_exp10(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  const abacus_float log102recip = 3.32192809488736234787031942948f;
  const abacus_float log102 = 0.301029995663981195213738894725f;

  // r = 10^x
  // r = 10^(k * log10(2) + f), k = floor(x / log10(2)), f = x - k * log10(2)
  // r = 10^(k * log10(2)) * 10^f
  // r = 2^k * 10^f
  const SignedType k = abacus::internal::floor_unsafe(x * log102recip);
  const T f = x - abacus::detail::cast::convert<T>(k) * log102;

  const abacus_float polynomial[3] = {1.00172475857780f, 2.18462045783410f,
                                      3.72095499205386f};

  const T twoToTheF = abacus::internal::horner_polynomial<T, 3>(f, polynomial);
  return abacus::internal::ldexp_unsafe(twoToTheF, k);
}
}  // namespace

abacus_float ABACUS_API __abacus_native_exp10(abacus_float x) {
  return native_exp10<>(x);
}
abacus_float2 ABACUS_API __abacus_native_exp10(abacus_float2 x) {
  return native_exp10<>(x);
}
abacus_float3 ABACUS_API __abacus_native_exp10(abacus_float3 x) {
  return native_exp10<>(x);
}
abacus_float4 ABACUS_API __abacus_native_exp10(abacus_float4 x) {
  return native_exp10<>(x);
}
abacus_float8 ABACUS_API __abacus_native_exp10(abacus_float8 x) {
  return native_exp10<>(x);
}
abacus_float16 ABACUS_API __abacus_native_exp10(abacus_float16 x) {
  return native_exp10<>(x);
}
