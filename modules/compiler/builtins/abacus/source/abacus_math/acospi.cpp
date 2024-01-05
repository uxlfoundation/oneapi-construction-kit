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
T acospi(const T x) {
  return __abacus_acos(x) * T(ABACUS_1_PI);
}

#ifdef __CA_BUILTINS_HALF_SUPPORT
// The half version loses too much accuracy by multiplying by just ABACUS_1_PI,
// so it needs its own implementation

// See the acospi sollya script for derivations
static ABACUS_CONSTANT abacus_half __codeplay_acospi_coeff_halfH1[3] = {
    -0.20263671875f16, 3.35693359375e-2f16, -1.08489990234375e-2f16};
static ABACUS_CONSTANT abacus_half __codeplay_acospi_coeff_halfH2[3] = {
    -0.318359375f16, -5.1300048828125e-2f16, -3.4820556640625e-2f16};

template <typename T>
T acospi_half(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  T xAbs = __abacus_fabs(x);
  const T x2 = x * x;
  T ans = 0.0f16;

  const SignedType cond1 = (xAbs > 5.9375E-1f16);

  xAbs = __abacus_select(xAbs, xAbs - 1.0f16, cond1);

  // TODO maybe there's a way to pick the coefficients of the polynomials
  // branchlessly, then this wouldn't branch
  const T poly1 =
      abacus::internal::horner_polynomial(x2, __codeplay_acospi_coeff_halfH2);
  const T poly2 =
      abacus::internal::horner_polynomial(xAbs, __codeplay_acospi_coeff_halfH1);

  ans = xAbs * __abacus_select(poly1, poly2, cond1);

  ans = __abacus_select(ans + 0.5f16, abacus::internal::sqrt(ans), cond1);

  ans = __abacus_select(ans, 1.0f16 - ans, x < 0.0f16);

  return ans;
}

template <>
abacus_half acospi_half(const abacus_half x) {
  abacus_half xAbs = __abacus_fabs(x);
  abacus_half ans = 0.0f16;

  if (xAbs > 5.9375E-1f16) {
    xAbs = xAbs - 1.0f16;
    ans = xAbs * abacus::internal::horner_polynomial(
                     xAbs, __codeplay_acospi_coeff_halfH1);

    ans = abacus::internal::sqrt(ans);
  } else {
    const abacus_half x2 = x * x;
    ans = xAbs * abacus::internal::horner_polynomial(
                     x2, __codeplay_acospi_coeff_halfH2);

    ans = ans + 0.5f16;
  }

  if (x < 0.0f16) {
    return 1.0f16 - ans;
  }

  return ans;
}
#endif  // __CA_BUILTINS_HALF_SUPPORT
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_acospi(abacus_half x) {
  return acospi_half<>(x);
}
abacus_half2 ABACUS_API __abacus_acospi(abacus_half2 x) {
  return acospi_half<>(x);
}
abacus_half3 ABACUS_API __abacus_acospi(abacus_half3 x) {
  return acospi_half<>(x);
}
abacus_half4 ABACUS_API __abacus_acospi(abacus_half4 x) {
  return acospi_half<>(x);
}
abacus_half8 ABACUS_API __abacus_acospi(abacus_half8 x) {
  return acospi_half<>(x);
}
abacus_half16 ABACUS_API __abacus_acospi(abacus_half16 x) {
  return acospi_half<>(x);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_acospi(abacus_float x) { return acospi<>(x); }
abacus_float2 ABACUS_API __abacus_acospi(abacus_float2 x) {
  return acospi<>(x);
}
abacus_float3 ABACUS_API __abacus_acospi(abacus_float3 x) {
  return acospi<>(x);
}
abacus_float4 ABACUS_API __abacus_acospi(abacus_float4 x) {
  return acospi<>(x);
}
abacus_float8 ABACUS_API __abacus_acospi(abacus_float8 x) {
  return acospi<>(x);
}
abacus_float16 ABACUS_API __abacus_acospi(abacus_float16 x) {
  return acospi<>(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_acospi(abacus_double x) {
  return acospi<>(x);
}
abacus_double2 ABACUS_API __abacus_acospi(abacus_double2 x) {
  return acospi<>(x);
}
abacus_double3 ABACUS_API __abacus_acospi(abacus_double3 x) {
  return acospi<>(x);
}
abacus_double4 ABACUS_API __abacus_acospi(abacus_double4 x) {
  return acospi<>(x);
}
abacus_double8 ABACUS_API __abacus_acospi(abacus_double8 x) {
  return acospi<>(x);
}
abacus_double16 ABACUS_API __abacus_acospi(abacus_double16 x) {
  return acospi<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
