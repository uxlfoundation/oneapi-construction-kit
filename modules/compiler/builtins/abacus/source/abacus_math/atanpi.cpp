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

namespace {
template <typename T>
T atanpi(const T x) {
  return __abacus_atan(x) * T(ABACUS_1_PI);
}

#ifdef __CA_BUILTINS_HALF_SUPPORT
// The half version loses too much accuracy by multiplying by just ABACUS_1_PI,
// so it needs its own implementation

// See the atanpi sollya script for derivations
static ABACUS_CONSTANT abacus_half __codeplay_atanpi_coeff_halfH1[5] = {
    0.318359375f16, -0.10552978515625f16, 5.682373046875e-2f16,
    -2.47955322265625e-2f16, 5.168914794921875e-3f16};

template <typename T>
T atanpi_half(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  T x_local = x;

  const SignedType inverse = (__abacus_fabs(x) >= 1.2f16);

  x_local = __abacus_select(x_local, 1.0f16 / x_local, inverse);

  const T x2 = x_local * x_local;
  T ans = x_local * abacus::internal::horner_polynomial(
                        x2, __codeplay_atanpi_coeff_halfH1);

  ans = __abacus_select(ans, __abacus_copysign(0.5f16, ans) - ans, inverse);

  return ans;
}

template <>
abacus_half atanpi_half(const abacus_half x) {
  abacus_half x_local = x;

  bool inverse = false;
  if (__abacus_fabs(x) >= 1.2f16) {
    inverse = 1;
    x_local = 1.0f16 / x_local;
  }

  const abacus_half x2 = x_local * x_local;
  abacus_half ans = x_local * abacus::internal::horner_polynomial(
                                  x2, __codeplay_atanpi_coeff_halfH1);

  if (!inverse) {
    return ans;
  }

  return __abacus_copysign(0.5f16, ans) - ans;
}
#endif  // __CA_BUILTINS_HALF_SUPPORT
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_atanpi(abacus_half x) {
  return atanpi_half<>(x);
}
abacus_half2 ABACUS_API __abacus_atanpi(abacus_half2 x) {
  return atanpi_half<>(x);
}
abacus_half3 ABACUS_API __abacus_atanpi(abacus_half3 x) {
  return atanpi_half<>(x);
}
abacus_half4 ABACUS_API __abacus_atanpi(abacus_half4 x) {
  return atanpi_half<>(x);
}
abacus_half8 ABACUS_API __abacus_atanpi(abacus_half8 x) {
  return atanpi_half<>(x);
}
abacus_half16 ABACUS_API __abacus_atanpi(abacus_half16 x) {
  return atanpi_half<>(x);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_atanpi(abacus_float x) { return atanpi<>(x); }
abacus_float2 ABACUS_API __abacus_atanpi(abacus_float2 x) {
  return atanpi<>(x);
}
abacus_float3 ABACUS_API __abacus_atanpi(abacus_float3 x) {
  return atanpi<>(x);
}
abacus_float4 ABACUS_API __abacus_atanpi(abacus_float4 x) {
  return atanpi<>(x);
}
abacus_float8 ABACUS_API __abacus_atanpi(abacus_float8 x) {
  return atanpi<>(x);
}
abacus_float16 ABACUS_API __abacus_atanpi(abacus_float16 x) {
  return atanpi<>(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_atanpi(abacus_double x) {
  return atanpi<>(x);
}
abacus_double2 ABACUS_API __abacus_atanpi(abacus_double2 x) {
  return atanpi<>(x);
}
abacus_double3 ABACUS_API __abacus_atanpi(abacus_double3 x) {
  return atanpi<>(x);
}
abacus_double4 ABACUS_API __abacus_atanpi(abacus_double4 x) {
  return atanpi<>(x);
}
abacus_double8 ABACUS_API __abacus_atanpi(abacus_double8 x) {
  return atanpi<>(x);
}
abacus_double16 ABACUS_API __abacus_atanpi(abacus_double16 x) {
  return atanpi<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
