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
#include <abacus/internal/fmod_unsafe.h>

namespace {
// Float: our fmod algorithm;
// 1. deconstructs both our input floats x & m
// 2. Figures out the difference in the exponents
// 3. * Uses the (a * b) % n = ((a % n) * (b % n)) % n equivalence
//    * a = x's mantissa, b = 2 ^ (x exponent - m exponent), n = m's mantissa
//    * Because we are dealing with mantissas they are at most 24 bits
//    * And (x exponent - m exponent) is at most 256
//    * We do at most 6 iterations shifting by 40 each time
//    * And then one cleanup for (x exponent - m exponent) % 40
// 4. Reconstructs the result from the new mantissa + m's exponent
template <typename T> T fmod_helper(const T x, const T m) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  const T result = abacus::internal::fmod_unsafe(x, m);

  const SignedType cond =
      (__abacus_isfinite(x) == 0) | __abacus_isnan(m) | (m == (T)0);
  return __abacus_select(__abacus_copysign(result, x), FPShape<T>::NaN(), cond);
}
} // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_fmod(abacus_half x, abacus_half m) {
  return fmod_helper(x, m);
}
abacus_half2 ABACUS_API __abacus_fmod(abacus_half2 x, abacus_half2 m) {
  return fmod_helper(x, m);
}
abacus_half3 ABACUS_API __abacus_fmod(abacus_half3 x, abacus_half3 m) {
  return fmod_helper(x, m);
}
abacus_half4 ABACUS_API __abacus_fmod(abacus_half4 x, abacus_half4 m) {
  return fmod_helper(x, m);
}
abacus_half8 ABACUS_API __abacus_fmod(abacus_half8 x, abacus_half8 m) {
  return fmod_helper(x, m);
}
abacus_half16 ABACUS_API __abacus_fmod(abacus_half16 x, abacus_half16 m) {
  return fmod_helper(x, m);
}
#endif // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_fmod(abacus_float x, abacus_float m) {
  return fmod_helper(x, m);
}
abacus_float2 ABACUS_API __abacus_fmod(abacus_float2 x, abacus_float2 m) {
  return fmod_helper(x, m);
}
abacus_float3 ABACUS_API __abacus_fmod(abacus_float3 x, abacus_float3 m) {
  return fmod_helper(x, m);
}
abacus_float4 ABACUS_API __abacus_fmod(abacus_float4 x, abacus_float4 m) {
  return fmod_helper(x, m);
}
abacus_float8 ABACUS_API __abacus_fmod(abacus_float8 x, abacus_float8 m) {
  return fmod_helper(x, m);
}
abacus_float16 ABACUS_API __abacus_fmod(abacus_float16 x, abacus_float16 m) {
  return fmod_helper(x, m);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_fmod(abacus_double x, abacus_double m) {
  return fmod_helper(x, m);
}
abacus_double2 ABACUS_API __abacus_fmod(abacus_double2 x, abacus_double2 m) {
  return fmod_helper(x, m);
}
abacus_double3 ABACUS_API __abacus_fmod(abacus_double3 x, abacus_double3 m) {
  return fmod_helper(x, m);
}
abacus_double4 ABACUS_API __abacus_fmod(abacus_double4 x, abacus_double4 m) {
  return fmod_helper(x, m);
}
abacus_double8 ABACUS_API __abacus_fmod(abacus_double8 x, abacus_double8 m) {
  return fmod_helper(x, m);
}
abacus_double16 ABACUS_API __abacus_fmod(abacus_double16 x, abacus_double16 m) {
  return fmod_helper(x, m);
}
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
