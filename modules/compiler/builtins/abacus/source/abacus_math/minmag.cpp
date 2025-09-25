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

template <typename T>
static T minmag_helper_scalar(const T x, const T y) {
  static_assert(TypeTraits<T>::num_elements == 1,
                "This function should only be called on scalar types");
  const T x_abs = __abacus_fabs(x);
  const T y_abs = __abacus_fabs(y);
  if (x_abs < y_abs) {
    return x;
  } else if (y_abs < x_abs) {
    return y;
  } else {
    return __abacus_fmin(x, y);
  }
}

template <typename T>
static T minmag_helper_vector(const T x, const T y) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  static_assert(TypeTraits<T>::num_elements != 1,
                "This function should only be called on vector types");
  const T x_abs = __abacus_fabs(x);
  const T y_abs = __abacus_fabs(y);

  const SignedType abs_x_lt_y_condition = (x_abs < y_abs);
  const SignedType abs_y_lt_x_condition = (y_abs < x_abs);
  const T absResult = __abacus_select(y, x, abs_x_lt_y_condition);
  return __abacus_select(__abacus_fmin(x, y), absResult,
                         (abs_x_lt_y_condition | abs_y_lt_x_condition));
}

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_minmag(abacus_half x, abacus_half y) {
  return minmag_helper_scalar(x, y);
}

abacus_half2 ABACUS_API __abacus_minmag(abacus_half2 x, abacus_half2 y) {
  return minmag_helper_vector(x, y);
}

abacus_half3 ABACUS_API __abacus_minmag(abacus_half3 x, abacus_half3 y) {
  return minmag_helper_vector(x, y);
}

abacus_half4 ABACUS_API __abacus_minmag(abacus_half4 x, abacus_half4 y) {
  return minmag_helper_vector(x, y);
}

abacus_half8 ABACUS_API __abacus_minmag(abacus_half8 x, abacus_half8 y) {
  return minmag_helper_vector(x, y);
}

abacus_half16 ABACUS_API __abacus_minmag(abacus_half16 x, abacus_half16 y) {
  return minmag_helper_vector(x, y);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_minmag(abacus_float x, abacus_float y) {
  return minmag_helper_scalar(x, y);
}

abacus_float2 ABACUS_API __abacus_minmag(abacus_float2 x, abacus_float2 y) {
  return minmag_helper_vector(x, y);
}

abacus_float3 ABACUS_API __abacus_minmag(abacus_float3 x, abacus_float3 y) {
  return minmag_helper_vector(x, y);
}

abacus_float4 ABACUS_API __abacus_minmag(abacus_float4 x, abacus_float4 y) {
  return minmag_helper_vector(x, y);
}

abacus_float8 ABACUS_API __abacus_minmag(abacus_float8 x, abacus_float8 y) {
  return minmag_helper_vector(x, y);
}

abacus_float16 ABACUS_API __abacus_minmag(abacus_float16 x, abacus_float16 y) {
  return minmag_helper_vector(x, y);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_minmag(abacus_double x, abacus_double y) {
  return minmag_helper_scalar(x, y);
}

abacus_double2 ABACUS_API __abacus_minmag(abacus_double2 x, abacus_double2 y) {
  return minmag_helper_vector(x, y);
}

abacus_double3 ABACUS_API __abacus_minmag(abacus_double3 x, abacus_double3 y) {
  return minmag_helper_vector(x, y);
}

abacus_double4 ABACUS_API __abacus_minmag(abacus_double4 x, abacus_double4 y) {
  return minmag_helper_vector(x, y);
}

abacus_double8 ABACUS_API __abacus_minmag(abacus_double8 x, abacus_double8 y) {
  return minmag_helper_vector(x, y);
}

abacus_double16 ABACUS_API __abacus_minmag(abacus_double16 x,
                                           abacus_double16 y) {
  return minmag_helper_vector(x, y);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
