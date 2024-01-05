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
#include <abacus/internal/horner_polynomial.h>
#include <abacus/internal/sqrt.h>

namespace {
template <typename T>
T asinpi(const T x) {
  return __abacus_asin(x) * T(ABACUS_1_PI);
}

#ifdef __CA_BUILTINS_HALF_SUPPORT
// The half version loses too much accuracy by multiplying by just ABACUS_1_PI,
// so it needs its own implementation

// See the asinpi sollya script for derivations
static ABACUS_CONSTANT abacus_half __codeplay_asinpi_coeff_halfH1[3] = {
    -0.20263671875f16, 3.35693359375e-2f16, -1.08489990234375e-2f16};
static ABACUS_CONSTANT abacus_half __codeplay_asinpi_coeff_halfH2[3] = {
    0.318359375f16, 5.1300048828125e-2f16, 3.4820556640625e-2f16};

template <typename T>
T asinpi_half(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  T xAbs = __abacus_fabs(x);
  const T x2 = x * x;

  T ans = x * abacus::internal::horner_polynomial(
                  x2, __codeplay_asinpi_coeff_halfH2);

  const SignedType cond1 = (xAbs > 5.9375E-1f16);

  // if(any(cond1)){ //A possible optimization, hardware dependent

  xAbs = xAbs - 1.0f16;

  T ansCond = xAbs * abacus::internal::horner_polynomial(
                         xAbs, __codeplay_asinpi_coeff_halfH1);
  ansCond = -abacus::internal::sqrt(ansCond) + 0.5f16;
  ansCond = __abacus_copysign(ansCond, x);

  ans = __abacus_select(ans, ansCond, cond1);

  //} // End of possible optimization.

  return ans;
}

template <>
abacus_half asinpi_half(const abacus_half x) {
  abacus_half xAbs = __abacus_fabs(x);

  // around x = 1 we want to estimate (asin(x) - pi/2)^2
  if (xAbs > 5.9375E-1f16) {
    xAbs = xAbs - 1.0f16;

    abacus_half ans = xAbs * abacus::internal::horner_polynomial(
                                 xAbs, __codeplay_asinpi_coeff_halfH1);

    ans = -abacus::internal::sqrt(ans) + 0.5f16;
    return __abacus_copysign(ans, x);
  }

  const abacus_half x2 = x * x;
  return x * abacus::internal::horner_polynomial(
                 x2, __codeplay_asinpi_coeff_halfH2);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_asinpi(abacus_half x) {
  return asinpi_half<>(x);
}
abacus_half2 ABACUS_API __abacus_asinpi(abacus_half2 x) {
  return asinpi_half<>(x);
}
abacus_half3 ABACUS_API __abacus_asinpi(abacus_half3 x) {
  return asinpi_half<>(x);
}
abacus_half4 ABACUS_API __abacus_asinpi(abacus_half4 x) {
  return asinpi_half<>(x);
}
abacus_half8 ABACUS_API __abacus_asinpi(abacus_half8 x) {
  return asinpi_half<>(x);
}
abacus_half16 ABACUS_API __abacus_asinpi(abacus_half16 x) {
  return asinpi_half<>(x);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_asinpi(abacus_float x) { return asinpi<>(x); }
abacus_float2 ABACUS_API __abacus_asinpi(abacus_float2 x) {
  return asinpi<>(x);
}
abacus_float3 ABACUS_API __abacus_asinpi(abacus_float3 x) {
  return asinpi<>(x);
}
abacus_float4 ABACUS_API __abacus_asinpi(abacus_float4 x) {
  return asinpi<>(x);
}
abacus_float8 ABACUS_API __abacus_asinpi(abacus_float8 x) {
  return asinpi<>(x);
}
abacus_float16 ABACUS_API __abacus_asinpi(abacus_float16 x) {
  return asinpi<>(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_asinpi(abacus_double x) {
  return asinpi<>(x);
}
abacus_double2 ABACUS_API __abacus_asinpi(abacus_double2 x) {
  return asinpi<>(x);
}
abacus_double3 ABACUS_API __abacus_asinpi(abacus_double3 x) {
  return asinpi<>(x);
}
abacus_double4 ABACUS_API __abacus_asinpi(abacus_double4 x) {
  return asinpi<>(x);
}
abacus_double8 ABACUS_API __abacus_asinpi(abacus_double8 x) {
  return asinpi<>(x);
}
abacus_double16 ABACUS_API __abacus_asinpi(abacus_double16 x) {
  return asinpi<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
