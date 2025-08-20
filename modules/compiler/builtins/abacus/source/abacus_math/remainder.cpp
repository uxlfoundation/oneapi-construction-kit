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
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T>
inline T remainder_helper(const T x, const T m) {
  typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type unused;
  return __abacus_remquo(x, m, &unused);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_remainder(abacus_half x, abacus_half m) {
  return remainder_helper(x, m);
}
abacus_half2 ABACUS_API __abacus_remainder(abacus_half2 x, abacus_half2 m) {
  return remainder_helper(x, m);
}
abacus_half3 ABACUS_API __abacus_remainder(abacus_half3 x, abacus_half3 m) {
  return remainder_helper(x, m);
}
abacus_half4 ABACUS_API __abacus_remainder(abacus_half4 x, abacus_half4 m) {
  return remainder_helper(x, m);
}
abacus_half8 ABACUS_API __abacus_remainder(abacus_half8 x, abacus_half8 m) {
  return remainder_helper(x, m);
}
abacus_half16 ABACUS_API __abacus_remainder(abacus_half16 x, abacus_half16 m) {
  return remainder_helper(x, m);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_remainder(abacus_float x, abacus_float m) {
  return remainder_helper(x, m);
}
abacus_float2 ABACUS_API __abacus_remainder(abacus_float2 x, abacus_float2 m) {
  return remainder_helper(x, m);
}
abacus_float3 ABACUS_API __abacus_remainder(abacus_float3 x, abacus_float3 m) {
  return remainder_helper(x, m);
}
abacus_float4 ABACUS_API __abacus_remainder(abacus_float4 x, abacus_float4 m) {
  return remainder_helper(x, m);
}
abacus_float8 ABACUS_API __abacus_remainder(abacus_float8 x, abacus_float8 m) {
  return remainder_helper(x, m);
}
abacus_float16 ABACUS_API __abacus_remainder(abacus_float16 x,
                                             abacus_float16 m) {
  return remainder_helper(x, m);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_remainder(abacus_double x, abacus_double m) {
  return remainder_helper(x, m);
}
abacus_double2 ABACUS_API __abacus_remainder(abacus_double2 x,
                                             abacus_double2 m) {
  return remainder_helper(x, m);
}
abacus_double3 ABACUS_API __abacus_remainder(abacus_double3 x,
                                             abacus_double3 m) {
  return remainder_helper(x, m);
}
abacus_double4 ABACUS_API __abacus_remainder(abacus_double4 x,
                                             abacus_double4 m) {
  return remainder_helper(x, m);
}
abacus_double8 ABACUS_API __abacus_remainder(abacus_double8 x,
                                             abacus_double8 m) {
  return remainder_helper(x, m);
}
abacus_double16 ABACUS_API __abacus_remainder(abacus_double16 x,
                                              abacus_double16 m) {
  return remainder_helper(x, m);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
