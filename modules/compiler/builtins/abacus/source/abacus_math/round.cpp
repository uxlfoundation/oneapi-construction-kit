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
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/is_integer_quick.h>

template <typename T>
static T round_helper_scalar(const T x) {
  static_assert(TypeTraits<T>::num_elements == 1,
                "Function should only be used for scalar types");
  typedef typename TypeTraits<T>::SignedType SignedType;
  if (abacus::internal::is_integer_quick(x)) {
    return x;
  }

  const SignedType toInt = abacus::detail::cast::convert<SignedType>(x);
  const T floored = abacus::detail::cast::convert<T>(toInt);
  const T increment = __abacus_fabs(floored) + T(1.0);
  const T ceiled = __abacus_copysign(increment, x);

  const bool to_floor = __abacus_fabs(floored - x) < T(0.5);
  return to_floor ? floored : ceiled;
}

template <typename T>
static T round_helper_vector(const T x) {
  static_assert(TypeTraits<T>::num_elements != 1,
                "Function should only be used for vector types");
  typedef typename TypeTraits<T>::SignedType SignedType;

  const SignedType toInt = abacus::detail::cast::convert<SignedType>(x);
  const T floored = abacus::detail::cast::convert<T>(toInt);
  const T increment = __abacus_fabs(floored) + T(1.0);
  const T ceiled = __abacus_copysign(increment, x);

  const SignedType to_floor = __abacus_fabs(floored - x) < T(0.5);
  const T result = __abacus_select(ceiled, floored, to_floor);
  return __abacus_select(result, x, abacus::internal::is_integer_quick(x));
}

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_round(abacus_half x) {
  return round_helper_scalar(x);
}
abacus_half2 ABACUS_API __abacus_round(abacus_half2 x) {
  return round_helper_vector(x);
}
abacus_half3 ABACUS_API __abacus_round(abacus_half3 x) {
  return round_helper_vector(x);
}
abacus_half4 ABACUS_API __abacus_round(abacus_half4 x) {
  return round_helper_vector(x);
}
abacus_half8 ABACUS_API __abacus_round(abacus_half8 x) {
  return round_helper_vector(x);
}
abacus_half16 ABACUS_API __abacus_round(abacus_half16 x) {
  return round_helper_vector(x);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_round(abacus_float x) {
  return round_helper_scalar(x);
}
abacus_float2 ABACUS_API __abacus_round(abacus_float2 x) {
  return round_helper_vector(x);
}
abacus_float3 ABACUS_API __abacus_round(abacus_float3 x) {
  return round_helper_vector(x);
}
abacus_float4 ABACUS_API __abacus_round(abacus_float4 x) {
  return round_helper_vector(x);
}
abacus_float8 ABACUS_API __abacus_round(abacus_float8 x) {
  return round_helper_vector(x);
}
abacus_float16 ABACUS_API __abacus_round(abacus_float16 x) {
  return round_helper_vector(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_round(abacus_double x) {
  return round_helper_scalar(x);
}
abacus_double2 ABACUS_API __abacus_round(abacus_double2 x) {
  return round_helper_vector(x);
}
abacus_double3 ABACUS_API __abacus_round(abacus_double3 x) {
  return round_helper_vector(x);
}
abacus_double4 ABACUS_API __abacus_round(abacus_double4 x) {
  return round_helper_vector(x);
}
abacus_double8 ABACUS_API __abacus_round(abacus_double8 x) {
  return round_helper_vector(x);
}
abacus_double16 ABACUS_API __abacus_round(abacus_double16 x) {
  return round_helper_vector(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
