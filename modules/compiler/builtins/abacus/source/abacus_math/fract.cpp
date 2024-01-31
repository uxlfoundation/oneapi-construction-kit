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

namespace {
template <typename T>
T biggestBelowOne();

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <>
abacus_half biggestBelowOne<abacus_half>() {
  return 0.999512f16;
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <>
abacus_float biggestBelowOne<abacus_float>() {
  return 0.999999940395355224609375f;
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <>
abacus_double biggestBelowOne<abacus_double>() {
  return 0.999999999999999888977697537484;
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
inline T fract_helper_scalar(T x, T *out) {
  static_assert(TypeTraits<T>::num_elements == 1,
                "Function should only be used for scalar types");

  const T whole_number = __abacus_floor(x);
  *out = whole_number;

  // This is the biggest fractional point below 1.
  const T biggestBelow1 =
      biggestBelowOne<typename TypeTraits<T>::ElementType>();

  T fract_part = __abacus_fmin(x - whole_number, biggestBelow1);
  fract_part = __abacus_isnan(x) ? x : fract_part;

  return __abacus_isinf(x) ? __abacus_copysign(T(0), x) : fract_part;
}

template <typename T>
inline T fract_helper_vector(T x, T *out) {
  static_assert(TypeTraits<T>::num_elements != 1,
                "Function should only be used for vector types");

  const T whole_number = __abacus_floor(x);
  *out = whole_number;

  // This is the biggest fractional point below 1.
  const T biggestBelow1 =
      biggestBelowOne<typename TypeTraits<T>::ElementType>();

  T fract_part = __abacus_fmin(x - whole_number, biggestBelow1);
  fract_part = __abacus_select(fract_part, x, __abacus_isnan(x));

  return __abacus_select(fract_part, __abacus_copysign(T(0), x),
                         __abacus_isinf(x));
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_fract(abacus_half x,
                                      abacus_half *out_whole_number) {
  return fract_helper_scalar(x, out_whole_number);
}
abacus_half2 ABACUS_API __abacus_fract(abacus_half2 x,
                                       abacus_half2 *out_whole_number) {
  return fract_helper_vector(x, out_whole_number);
}
abacus_half3 ABACUS_API __abacus_fract(abacus_half3 x,
                                       abacus_half3 *out_whole_number) {
  return fract_helper_vector(x, out_whole_number);
}
abacus_half4 ABACUS_API __abacus_fract(abacus_half4 x,
                                       abacus_half4 *out_whole_number) {
  return fract_helper_vector(x, out_whole_number);
}
abacus_half8 ABACUS_API __abacus_fract(abacus_half8 x,
                                       abacus_half8 *out_whole_number) {
  return fract_helper_vector(x, out_whole_number);
}
abacus_half16 ABACUS_API __abacus_fract(abacus_half16 x,
                                        abacus_half16 *out_whole_number) {
  return fract_helper_vector(x, out_whole_number);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_fract(abacus_float x,
                                       abacus_float *out_whole_number) {
  return fract_helper_scalar(x, out_whole_number);
}
abacus_float2 ABACUS_API __abacus_fract(abacus_float2 x,
                                        abacus_float2 *out_whole_number) {
  return fract_helper_vector(x, out_whole_number);
}
abacus_float3 ABACUS_API __abacus_fract(abacus_float3 x,
                                        abacus_float3 *out_whole_number) {
  return fract_helper_vector(x, out_whole_number);
}
abacus_float4 ABACUS_API __abacus_fract(abacus_float4 x,
                                        abacus_float4 *out_whole_number) {
  return fract_helper_vector(x, out_whole_number);
}
abacus_float8 ABACUS_API __abacus_fract(abacus_float8 x,
                                        abacus_float8 *out_whole_number) {
  return fract_helper_vector(x, out_whole_number);
}
abacus_float16 ABACUS_API __abacus_fract(abacus_float16 x,
                                         abacus_float16 *out_whole_number) {
  return fract_helper_vector(x, out_whole_number);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_fract(abacus_double x,
                                        abacus_double *out_whole_number) {
  return fract_helper_scalar(x, out_whole_number);
}
abacus_double2 ABACUS_API __abacus_fract(abacus_double2 x,
                                         abacus_double2 *out_whole_number) {
  return fract_helper_vector(x, out_whole_number);
}
abacus_double3 ABACUS_API __abacus_fract(abacus_double3 x,
                                         abacus_double3 *out_whole_number) {
  return fract_helper_vector(x, out_whole_number);
}
abacus_double4 ABACUS_API __abacus_fract(abacus_double4 x,
                                         abacus_double4 *out_whole_number) {
  return fract_helper_vector(x, out_whole_number);
}
abacus_double8 ABACUS_API __abacus_fract(abacus_double8 x,
                                         abacus_double8 *out_whole_number) {
  return fract_helper_vector(x, out_whole_number);
}
abacus_double16 ABACUS_API __abacus_fract(abacus_double16 x,
                                          abacus_double16 *out_whole_number) {
  return fract_helper_vector(x, out_whole_number);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
