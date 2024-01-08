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
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/pow_unsafe.h>

namespace {
template <typename T>
inline T powr(const T x, const T y) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  const SignedType xIsInf = __abacus_isinf(x);
  const SignedType yIsInf = __abacus_isinf(y);

  T result = abacus::internal::pow_unsafe(x, y);

  // Return 0 in the following conditions:
  // * x is +/- zero and y is positive
  //
  // We use the xor here because pow(0, +inf) is zero, pow(0, -inf) is
  // inf, and pow(inf, inf) is inf
  const T bit = __abacus_select(T(ABACUS_INFINITY), T(0),
                                SignedType((x < 1.0f) ^ (y < 0.0f)));

  // Return positive INFINITY in the following conditions:
  // * x is INFINITY
  // * x is +/- zero and y is negative INFINITY
  // * x is +/- zero and y is negative finite value
  result =
      __abacus_select(result, bit, SignedType(xIsInf | yIsInf | (x == 0.0f)));

  // Return 1 for any input where x is 1 or y is zero and we do not subsequently
  // change it to NaN.
  result =
      __abacus_select(result, T(1.0f), SignedType((x == 1.0f) | (y == 0.0f)));

  // Return NaN in the following conditions:
  // * x is NaN
  // * y is NaN
  // * x is less than zero
  // * x is +/- zero and y is +/- zero
  // * x is INFINITY and y is +/- zero
  // * x is 1.0 and y is INFINITY
  const SignedType nan_cond = (x < 0.0f) | __abacus_isnan(x) |
                              __abacus_isnan(y) | ((x == 0.0f) & (y == 0.0f)) |
                              (xIsInf & (y == 0.0f)) | (yIsInf & (x == 1.0f));

  result = __abacus_select(result, FPShape<T>::NaN(), nan_cond);

  return result;
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_powr(abacus_half x, abacus_half y) {
  return powr<>(x, y);
}
abacus_half2 ABACUS_API __abacus_powr(abacus_half2 x, abacus_half2 y) {
  return powr<>(x, y);
}
abacus_half3 ABACUS_API __abacus_powr(abacus_half3 x, abacus_half3 y) {
  return powr<>(x, y);
}
abacus_half4 ABACUS_API __abacus_powr(abacus_half4 x, abacus_half4 y) {
  return powr<>(x, y);
}
abacus_half8 ABACUS_API __abacus_powr(abacus_half8 x, abacus_half8 y) {
  return powr<>(x, y);
}
abacus_half16 ABACUS_API __abacus_powr(abacus_half16 x, abacus_half16 y) {
  return powr<>(x, y);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_powr(abacus_float x, abacus_float y) {
  return powr<>(x, y);
}
abacus_float2 ABACUS_API __abacus_powr(abacus_float2 x, abacus_float2 y) {
  return powr<>(x, y);
}
abacus_float3 ABACUS_API __abacus_powr(abacus_float3 x, abacus_float3 y) {
  return powr<>(x, y);
}
abacus_float4 ABACUS_API __abacus_powr(abacus_float4 x, abacus_float4 y) {
  return powr<>(x, y);
}
abacus_float8 ABACUS_API __abacus_powr(abacus_float8 x, abacus_float8 y) {
  return powr<>(x, y);
}
abacus_float16 ABACUS_API __abacus_powr(abacus_float16 x, abacus_float16 y) {
  return powr<>(x, y);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_powr(abacus_double x, abacus_double y) {
  return powr<>(x, y);
}
abacus_double2 ABACUS_API __abacus_powr(abacus_double2 x, abacus_double2 y) {
  return powr<>(x, y);
}
abacus_double3 ABACUS_API __abacus_powr(abacus_double3 x, abacus_double3 y) {
  return powr<>(x, y);
}
abacus_double4 ABACUS_API __abacus_powr(abacus_double4 x, abacus_double4 y) {
  return powr<>(x, y);
}
abacus_double8 ABACUS_API __abacus_powr(abacus_double8 x, abacus_double8 y) {
  return powr<>(x, y);
}
abacus_double16 ABACUS_API __abacus_powr(abacus_double16 x, abacus_double16 y) {
  return powr<>(x, y);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
