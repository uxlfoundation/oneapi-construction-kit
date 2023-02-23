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
T native_exp(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  const abacus_float ln2recip =
      1.44269504088896340735992468100189213742664595415298593413544f;
  const abacus_float ln2 = 0.693147180559945309417232121458f;

  // r = e^x
  // r = e^(k * ln(2) + f), k = floor(x / ln(2)), f = x - k * ln(2)
  // r = e^(k * ln(2)) * e^f
  // r = 2^k * e^f
  const SignedType k = abacus::internal::floor_unsafe(x * ln2recip);
  const T f = x - abacus::detail::cast::convert<T>(k) * ln2;

  const abacus_float polynomial[3] = {1.00172475857779f, .948768609890313f,
                                      .701815635555134f};

  const T twoToTheF = abacus::internal::horner_polynomial<T, 3>(f, polynomial);
  return abacus::internal::ldexp_unsafe(twoToTheF, k);
}
}  // namespace

abacus_float ABACUS_API __abacus_native_exp(abacus_float x) {
  return native_exp<>(x);
}
abacus_float2 ABACUS_API __abacus_native_exp(abacus_float2 x) {
  return native_exp<>(x);
}
abacus_float3 ABACUS_API __abacus_native_exp(abacus_float3 x) {
  return native_exp<>(x);
}
abacus_float4 ABACUS_API __abacus_native_exp(abacus_float4 x) {
  return native_exp<>(x);
}
abacus_float8 ABACUS_API __abacus_native_exp(abacus_float8 x) {
  return native_exp<>(x);
}
abacus_float16 ABACUS_API __abacus_native_exp(abacus_float16 x) {
  return native_exp<>(x);
}
