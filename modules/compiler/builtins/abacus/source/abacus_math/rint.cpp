// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/internal/is_denorm.h>
#include <abacus/internal/is_integer_quick.h>
#include <abacus/internal/trunc_unsafe.h>

template <typename T>
static T rint_helper_scalar(const T x) {
  static_assert(TypeTraits<T>::num_elements == 1,
                "Function should only be used for scalar types");
  typedef typename TypeTraits<T>::SignedType SignedType;

  if (abacus::internal::is_denorm(x)) {
    return T(0.0);
  } else if (!__abacus_isnormal(x)) {
    return x;
  } else if (abacus::internal::is_integer_quick(x)) {
    return x;
  }

  const T xAbs = __abacus_fabs(x);
  const SignedType xTruncated = abacus::internal::trunc_unsafe(xAbs);
  const T xTrunc = abacus::detail::cast::convert<T>(xTruncated);
  const T diff = (xAbs - xTrunc);

  T result = xTrunc;

  if ((diff > T(0.5)) || (diff == T(0.5) && (1 & xTruncated))) {
    result += T(1.0);
  }

  return __abacus_copysign(result, x);
}

template <typename T>
static T rint_helper_vector(const T x) {
  static_assert(TypeTraits<T>::num_elements != 1,
                "Function should only be used for vector types");
  typedef typename TypeTraits<T>::SignedType SignedType;
  const T zero(0.0);
  const T onehalf(0.5);
  const T one(1.0);

  const T xAbs = __abacus_fabs(x);
  const SignedType xTruncated = abacus::detail::cast::convert<SignedType>(xAbs);
  const T xTrunc = abacus::detail::cast::convert<T>(xTruncated);
  const T diff = (xAbs - xTrunc);

  T r = __abacus_select(
      xTrunc, xTrunc + one,
      (diff > onehalf) | ((diff == onehalf) &
                          ((SignedType)1 == (xTruncated & (SignedType)1))));

  r = __abacus_copysign(r, x);

  r = __abacus_select(
      r, x, ~__abacus_isnormal(x) | abacus::internal::is_integer_quick(x));

  return __abacus_select(r, zero, abacus::internal::is_denorm(x));
}

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_rint(abacus_half x) {
  return rint_helper_scalar(x);
}
abacus_half2 ABACUS_API __abacus_rint(abacus_half2 x) {
  return rint_helper_vector(x);
}
abacus_half3 ABACUS_API __abacus_rint(abacus_half3 x) {
  return rint_helper_vector(x);
}
abacus_half4 ABACUS_API __abacus_rint(abacus_half4 x) {
  return rint_helper_vector(x);
}
abacus_half8 ABACUS_API __abacus_rint(abacus_half8 x) {
  return rint_helper_vector(x);
}
abacus_half16 ABACUS_API __abacus_rint(abacus_half16 x) {
  return rint_helper_vector(x);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_rint(abacus_float x) {
  return rint_helper_scalar(x);
}
abacus_float2 ABACUS_API __abacus_rint(abacus_float2 x) {
  return rint_helper_vector(x);
}
abacus_float3 ABACUS_API __abacus_rint(abacus_float3 x) {
  return rint_helper_vector(x);
}
abacus_float4 ABACUS_API __abacus_rint(abacus_float4 x) {
  return rint_helper_vector(x);
}
abacus_float8 ABACUS_API __abacus_rint(abacus_float8 x) {
  return rint_helper_vector(x);
}
abacus_float16 ABACUS_API __abacus_rint(abacus_float16 x) {
  return rint_helper_vector(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_rint(abacus_double x) {
  return rint_helper_scalar(x);
}
abacus_double2 ABACUS_API __abacus_rint(abacus_double2 x) {
  return rint_helper_vector(x);
}
abacus_double3 ABACUS_API __abacus_rint(abacus_double3 x) {
  return rint_helper_vector(x);
}
abacus_double4 ABACUS_API __abacus_rint(abacus_double4 x) {
  return rint_helper_vector(x);
}
abacus_double8 ABACUS_API __abacus_rint(abacus_double8 x) {
  return rint_helper_vector(x);
}
abacus_double16 ABACUS_API __abacus_rint(abacus_double16 x) {
  return rint_helper_vector(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
