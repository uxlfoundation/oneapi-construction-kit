// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/is_odd.h>
#include <abacus/internal/lgamma_positive.h>

namespace {
template <typename T>
T lgamma_r(const T x,
           typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type
               *out_sign) {
  using IntType =
      typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type;
  using SignedType = typename TypeTraits<T>::SignedType;
  using UnsignedType = typename TypeTraits<T>::UnsignedType;
  using Traits = abacus::internal::lgamma_traits<T>;

  const T posResult = abacus::internal::lgamma_positive(__abacus_fabs(x));

  // gamma(x) result is positive for:
  // - positive values of x
  // - odd negative values of x
  IntType sign =
      __abacus_select(IntType(-1), IntType(1),
                      abacus::detail::cast::convert<IntType>(x > T(0)) |
                          (abacus::detail::cast::convert<IntType>(x < T(0)) &
                           abacus::detail::cast::convert<IntType>(
                               abacus::internal::is_odd(x))));

  const T sin_pi = __abacus_sinpi(x);

  // Negative derived by Eulers reflection formula:
  // gamma(x) gamma(1-x) = pi / sinpi(x)
  const T euler = (x * T(Traits::one_over_pi)) * sin_pi;
  T result = -(posResult + __abacus_log(__abacus_fabs(euler)));

  result = __abacus_select(result, posResult, SignedType(x > T(0)));

  SignedType xRetZero = (x <= T(0)) & (__abacus_floor(x) == x);
  if (__abacus_isftz()) {
    // If a device doesn't support denormals rather than trying to find
    // the correct answer by scaling we'll just treat the denormal as
    // zero. The lgamma_r ULP requirement is stated as undefined, so as long as
    // we don't return NAN here this is correct.

    // x is a denormal or zero if exponent bits are all zero
    const SignedType xAsInt = abacus::detail::cast::as<SignedType>(x);
    xRetZero |= SignedType(0) == (xAsInt & FPShape<T>::ExponentMask());
  }

  result = __abacus_select(result, T(0), xRetZero);

  const SignedType xRetNan = __abacus_isnan(x);
  result = __abacus_select(result, x, xRetNan);

  const SignedType xRetInf =
      x > abacus::detail::cast::as<T>(UnsignedType(Traits::overflow_limit));
  result = __abacus_select(result, T(ABACUS_INFINITY), xRetInf);

  sign = __abacus_select(
      sign, IntType(0),
      abacus::detail::cast::convert<IntType>(xRetNan | xRetZero));
  *out_sign = sign;

  return result;
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_lgamma_r(abacus_half x, abacus_int *out_sign) {
  return lgamma_r<>(x, out_sign);
}
abacus_half2 ABACUS_API __abacus_lgamma_r(abacus_half2 x,
                                          abacus_int2 *out_sign) {
  return lgamma_r<>(x, out_sign);
}
abacus_half3 ABACUS_API __abacus_lgamma_r(abacus_half3 x,
                                          abacus_int3 *out_sign) {
  return lgamma_r<>(x, out_sign);
}
abacus_half4 ABACUS_API __abacus_lgamma_r(abacus_half4 x,
                                          abacus_int4 *out_sign) {
  return lgamma_r<>(x, out_sign);
}
abacus_half8 ABACUS_API __abacus_lgamma_r(abacus_half8 x,
                                          abacus_int8 *out_sign) {
  return lgamma_r<>(x, out_sign);
}
abacus_half16 ABACUS_API __abacus_lgamma_r(abacus_half16 x,
                                           abacus_int16 *out_sign) {
  return lgamma_r<>(x, out_sign);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_lgamma_r(abacus_float x,
                                          abacus_int *out_sign) {
  return lgamma_r<>(x, out_sign);
}
abacus_float2 ABACUS_API __abacus_lgamma_r(abacus_float2 x,
                                           abacus_int2 *out_sign) {
  return lgamma_r<>(x, out_sign);
}
abacus_float3 ABACUS_API __abacus_lgamma_r(abacus_float3 x,
                                           abacus_int3 *out_sign) {
  return lgamma_r<>(x, out_sign);
}
abacus_float4 ABACUS_API __abacus_lgamma_r(abacus_float4 x,
                                           abacus_int4 *out_sign) {
  return lgamma_r<>(x, out_sign);
}
abacus_float8 ABACUS_API __abacus_lgamma_r(abacus_float8 x,
                                           abacus_int8 *out_sign) {
  return lgamma_r<>(x, out_sign);
}
abacus_float16 ABACUS_API __abacus_lgamma_r(abacus_float16 x,
                                            abacus_int16 *out_sign) {
  return lgamma_r<>(x, out_sign);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_lgamma_r(abacus_double x,
                                           abacus_int *out_sign) {
  *out_sign = 0;

  if (__abacus_isnan(x)) {
    return ABACUS_NAN;
  }

  if (x == ABACUS_INFINITY || x == 0.0 || x < -9.0071993e+15) {
    return ABACUS_INFINITY;
  }

  // gamma(x) result is positive for:
  // - positive values of x
  // - odd negative values of x
  if ((x > 0.0) || ((x < 0.0) && abacus::internal::is_odd(x))) {
    *out_sign = 1;
  }

  // Values that would underflow or overflow later in the algorithm:
  if (-1.0e-15 <= x && x < 0.0) {
    return -1.0 * __abacus_log(-x);
  }

  const abacus_double xAbs = __abacus_fabs(x);
  const abacus_double posResult = abacus::internal::lgamma_positive(xAbs);

  if (x > 0.0) {
    return posResult;
  }

  const abacus_double sin_pi = __abacus_sinpi(x);

  // the ol catastrophic cancellation. Good thing we have inf ulps to work with:
  // If x is small x/pi underflows:
  return -(posResult + __abacus_log(__abacus_fabs((ABACUS_1_PI * x) * sin_pi)));
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

namespace {
template <typename T>
T lgamma_r_splat(
    const T x,
    typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type *o) {
  T result;
  for (unsigned i = 0; i < TypeTraits<T>::num_elements; i++) {
    abacus_int tmp;
    result[i] = __abacus_lgamma_r(x[i], &tmp);
    (*o)[i] = tmp;
  }
  return result;
}
}  // namespace

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double2 ABACUS_API __abacus_lgamma_r(abacus_double2 x, abacus_int2 *o) {
  return lgamma_r_splat<>(x, o);
}
abacus_double3 ABACUS_API __abacus_lgamma_r(abacus_double3 x, abacus_int3 *o) {
  return lgamma_r_splat<>(x, o);
}
abacus_double4 ABACUS_API __abacus_lgamma_r(abacus_double4 x, abacus_int4 *o) {
  return lgamma_r_splat<>(x, o);
}
abacus_double8 ABACUS_API __abacus_lgamma_r(abacus_double8 x, abacus_int8 *o) {
  return lgamma_r_splat<>(x, o);
}
abacus_double16 ABACUS_API __abacus_lgamma_r(abacus_double16 x,
                                             abacus_int16 *o) {
  return lgamma_r_splat<>(x, o);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
