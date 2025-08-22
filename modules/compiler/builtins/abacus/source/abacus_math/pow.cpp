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
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/is_odd.h>
#include <abacus/internal/pow_unsafe.h>

namespace {
template <typename T> inline T pow(const T x, const T y) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  const T xAbs = __abacus_fabs(x);

  const SignedType yIsInt = (y == __abacus_floor(y));

  const SignedType yIsOddInt = yIsInt & abacus::internal::is_odd(y);
  const SignedType yIsEvenInt = yIsInt & ~yIsOddInt;

  const SignedType xIsNegative = __abacus_signbit(x);
  const SignedType yIsNegative = __abacus_signbit(y);

  const SignedType xIsInf = __abacus_isinf(x);
  const SignedType yIsInf = __abacus_isinf(y);

  const SignedType xAbsInt = abacus::detail::cast::as<SignedType>(xAbs);

  T result = abacus::internal::pow_unsafe(xAbs, y);

  result =
      __abacus_select(result, -result, SignedType(xIsNegative & yIsOddInt));

  result = __abacus_select(result, T(0),
                           SignedType(xIsInf | yIsInf | (xAbsInt == 0)));

  // Return positive INFINITY in the following conditions:
  // * x has an absolute value greater than 1, and y is positive INFINITY
  // * x is +/- zero and y is negative
  // * x has an absolute value less than 1.0 and y is negative INFINITY
  const SignedType pos_inf_cond =
      ((xAbs > 1.0f) & ~yIsNegative & yIsInf) |
      (yIsNegative & (((xAbs < T(1.0)) & yIsInf) | (xAbsInt == 0))) |
      (~yIsNegative &
       ((xIsInf & (yIsEvenInt | ~yIsInt)) | (~xIsNegative & xIsInf)));

  result = __abacus_select(result, T(ABACUS_INFINITY), pos_inf_cond);

  // Return negative INFINITY in the following conditions:
  // * x is negative zero and y is a negative odd integer value
  // * x is negative infinity and y is a positive odd integer value
  const SignedType neg_inf_cond =
      ((abacus::detail::cast::as<SignedType>(x) ==
        TypeTraits<SignedType>::min()) &
       yIsNegative & yIsOddInt) |
      (xIsNegative & xIsInf & ~yIsNegative & yIsOddInt);
  result = __abacus_select(result, T(-ABACUS_INFINITY), neg_inf_cond);

  // Return NAN in the following conditions:
  // * x is NAN
  // * y is NAN
  // * x a negative non-zero, non-infinity value and y is a fractional number
  const SignedType nan_cond =
      __abacus_isnan(x) | __abacus_isnan(y) |
      ((xAbsInt != 0) & xIsNegative & ~yIsInt & ~xIsInf);

  result = __abacus_select(result, FPShape<T>::NaN(), nan_cond);

  // Return 1 in the following conditions:
  // * x is 1
  // * y is +/- 0.0
  // * x is -1 and y is INFINITY
  // * x is -1 and y is an even integer value
  const SignedType one_cond =
      (x == T(1.0)) | (y == T(0)) | ((x == T(-1.0)) & (yIsEvenInt | yIsInf));
  result = __abacus_select(result, T(1.0), one_cond);

  return result;
}
} // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_pow(abacus_half x, abacus_half y) {
  return pow<>(x, y);
}
abacus_half2 ABACUS_API __abacus_pow(abacus_half2 x, abacus_half2 y) {
  return pow<>(x, y);
}
abacus_half3 ABACUS_API __abacus_pow(abacus_half3 x, abacus_half3 y) {
  return pow<>(x, y);
}
abacus_half4 ABACUS_API __abacus_pow(abacus_half4 x, abacus_half4 y) {
  return pow<>(x, y);
}
abacus_half8 ABACUS_API __abacus_pow(abacus_half8 x, abacus_half8 y) {
  return pow<>(x, y);
}
abacus_half16 ABACUS_API __abacus_pow(abacus_half16 x, abacus_half16 y) {
  return pow<>(x, y);
}
#endif // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_pow(abacus_float x, abacus_float y) {
  return pow<>(x, y);
}
abacus_float2 ABACUS_API __abacus_pow(abacus_float2 x, abacus_float2 y) {
  return pow<>(x, y);
}
abacus_float3 ABACUS_API __abacus_pow(abacus_float3 x, abacus_float3 y) {
  return pow<>(x, y);
}
abacus_float4 ABACUS_API __abacus_pow(abacus_float4 x, abacus_float4 y) {
  return pow<>(x, y);
}
abacus_float8 ABACUS_API __abacus_pow(abacus_float8 x, abacus_float8 y) {
  return pow<>(x, y);
}
abacus_float16 ABACUS_API __abacus_pow(abacus_float16 x, abacus_float16 y) {
  return pow<>(x, y);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_pow(abacus_double x, abacus_double y) {
  return pow<>(x, y);
}
abacus_double2 ABACUS_API __abacus_pow(abacus_double2 x, abacus_double2 y) {
  return pow<>(x, y);
}
abacus_double3 ABACUS_API __abacus_pow(abacus_double3 x, abacus_double3 y) {
  return pow<>(x, y);
}
abacus_double4 ABACUS_API __abacus_pow(abacus_double4 x, abacus_double4 y) {
  return pow<>(x, y);
}
abacus_double8 ABACUS_API __abacus_pow(abacus_double8 x, abacus_double8 y) {
  return pow<>(x, y);
}
abacus_double16 ABACUS_API __abacus_pow(abacus_double16 x, abacus_double16 y) {
  return pow<>(x, y);
}
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
