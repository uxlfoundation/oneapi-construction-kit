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
T native_exp2(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  // r = 2^x
  // r = 2^(k + f), k = floor(x), f = x - floor(f)
  // r = 2^k * 2^f
  const SignedType k = abacus::internal::floor_unsafe(x);
  const T f = x - abacus::detail::cast::convert<T>(k);

  const abacus_float polynomial[3] = {1.00172475857779f, .657636286949233f,
                                      .337189437317397f};

  const T twoToTheF = abacus::internal::horner_polynomial<T, 3>(f, polynomial);
  return abacus::internal::ldexp_unsafe(twoToTheF, k);
}
}  // namespace

abacus_float ABACUS_API __abacus_native_exp2(abacus_float x) {
  return native_exp2<>(x);
}
abacus_float2 ABACUS_API __abacus_native_exp2(abacus_float2 x) {
  return native_exp2<>(x);
}
abacus_float3 ABACUS_API __abacus_native_exp2(abacus_float3 x) {
  return native_exp2<>(x);
}
abacus_float4 ABACUS_API __abacus_native_exp2(abacus_float4 x) {
  return native_exp2<>(x);
}
abacus_float8 ABACUS_API __abacus_native_exp2(abacus_float8 x) {
  return native_exp2<>(x);
}
abacus_float16 ABACUS_API __abacus_native_exp2(abacus_float16 x) {
  return native_exp2<>(x);
}
