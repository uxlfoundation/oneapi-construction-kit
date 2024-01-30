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
#include <abacus/internal/lgamma_positive.h>

namespace {
template <typename T>
T lgamma_vector(const T x) {
  using UnsignedType = typename TypeTraits<T>::UnsignedType;
  using Traits = abacus::internal::lgamma_traits<T>;
  static_assert(1 != TypeTraits<T>::num_elements,
                "Function should only be instantiated for vector types");

  const T posResult = abacus::internal::lgamma_positive(__abacus_fabs(x));

  // Negative derived by Eulers reflection formula:
  // gamma(x) gamma(1-x) = pi / sinpi(x)
  const T euler = (T(Traits::one_over_pi) * x) * __abacus_sinpi(x);
  const T negResult = -(posResult + __abacus_log(__abacus_fabs(euler)));

  T result = __abacus_select(negResult, posResult, x >= T(0));

  const T overflow_limit =
      abacus::detail::cast::as<T>(UnsignedType(Traits::overflow_limit));
  const T underflow_limit =
      abacus::detail::cast::as<T>(UnsignedType(Traits::underflow_limit));

  result = __abacus_select(
      result, ABACUS_INFINITY,
      (x > overflow_limit) | (x <= underflow_limit) | (x == T(0)));

  result = __abacus_select(result, x, __abacus_isnan(x));

  return result;
}

template <typename T>
T lgamma_scalar(const T x) {
  using UnsignedType = typename TypeTraits<T>::UnsignedType;
  using Traits = abacus::internal::lgamma_traits<T>;
  static_assert(1 == TypeTraits<T>::num_elements,
                "Function should only be instantiated for scalar types");
  if (__abacus_isnan(x)) {
    return x;
  }

  const T overflow_limit =
      abacus::detail::cast::as<T>(UnsignedType(Traits::overflow_limit));
  const T underflow_limit =
      abacus::detail::cast::as<T>(UnsignedType(Traits::underflow_limit));

  if ((x > overflow_limit) || (x <= underflow_limit) || (x == T(0))) {
    // for the low x case, number goes complex. Should that become NaN?
    return ABACUS_INFINITY;
  }

  const T xPositive = __abacus_fabs(x);

  const T posResult = abacus::internal::lgamma_positive(xPositive);

  if (x >= T(0)) {
    return posResult;
  }

  // Negative derived by Eulers reflection formula:
  // gamma(x) gamma(1-x) = pi / sinpi(x)
  const T euler = (Traits::one_over_pi * x) * __abacus_sinpi(x);
  return -(posResult + __abacus_log(__abacus_fabs(euler)));
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_lgamma(abacus_half x) {
  return lgamma_scalar(x);
}
abacus_half2 ABACUS_API __abacus_lgamma(abacus_half2 x) {
  return lgamma_vector(x);
}
abacus_half3 ABACUS_API __abacus_lgamma(abacus_half3 x) {
  return lgamma_vector(x);
}
abacus_half4 ABACUS_API __abacus_lgamma(abacus_half4 x) {
  return lgamma_vector(x);
}
abacus_half8 ABACUS_API __abacus_lgamma(abacus_half8 x) {
  return lgamma_vector(x);
}
abacus_half16 ABACUS_API __abacus_lgamma(abacus_half16 x) {
  return lgamma_vector(x);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_lgamma(abacus_float x) {
  return lgamma_scalar(x);
}
abacus_float2 ABACUS_API __abacus_lgamma(abacus_float2 x) {
  return lgamma_vector(x);
}
abacus_float3 ABACUS_API __abacus_lgamma(abacus_float3 x) {
  return lgamma_vector(x);
}
abacus_float4 ABACUS_API __abacus_lgamma(abacus_float4 x) {
  return lgamma_vector(x);
}
abacus_float8 ABACUS_API __abacus_lgamma(abacus_float8 x) {
  return lgamma_vector(x);
}
abacus_float16 ABACUS_API __abacus_lgamma(abacus_float16 x) {
  return lgamma_vector(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_lgamma(abacus_double x) {
  if (__abacus_isnan(x)) {
    return ABACUS_NAN;
  }

  if (x == ABACUS_INFINITY || x == 0.0 || x < -9.0071993e+15) {
    return ABACUS_INFINITY;
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

  // the ol catastrophic cancellation. Good thing we have inf ulps to work with:
  // If x is small x/pi underflows:
  return -(posResult +
           __abacus_log(__abacus_fabs((ABACUS_1_PI * x) * __abacus_sinpi(x))));
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

namespace {
template <typename T>
T lgamma_splat(const T x) {
  T result;
  for (unsigned i = 0; i < TypeTraits<T>::num_elements; i++) {
    result[i] = __abacus_lgamma(x[i]);
  }
  return result;
}
}  // namespace

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double2 ABACUS_API __abacus_lgamma(abacus_double2 x) {
  return lgamma_splat<>(x);
}
abacus_double3 ABACUS_API __abacus_lgamma(abacus_double3 x) {
  return lgamma_splat<>(x);
}
abacus_double4 ABACUS_API __abacus_lgamma(abacus_double4 x) {
  return lgamma_splat<>(x);
}
abacus_double8 ABACUS_API __abacus_lgamma(abacus_double8 x) {
  return lgamma_splat<>(x);
}
abacus_double16 ABACUS_API __abacus_lgamma(abacus_double16 x) {
  return lgamma_splat<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
