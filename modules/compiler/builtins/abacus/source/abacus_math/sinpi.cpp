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
#include <abacus/internal/is_odd.h>

#ifdef __CA_BUILTINS_HALF_SUPPORT
#include <abacus/internal/add_exact.h>
#include <abacus/internal/multiply_exact.h>
#endif

namespace {

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct helper<T, abacus_half> {
  static T _(const T x) {
    // To reduce precision errors, we evaluate the horner polynomial excluding
    // the final multiply-add, then manually perform the final step using
    // multiply_exact / add_exact.
    const abacus_half polynomial[3] = {3.140625f16, -5.13671875f16,
                                       2.298828125f16};
    const T poly = abacus::internal::horner_polynomial(x, &polynomial[1], 2);

    // Perform last horner_polynomial step by hand.
    T poly_mul_lo;
    const T poly_mul_hi =
        abacus::internal::multiply_exact<T>(x, poly, &poly_mul_lo);

    // For all possible inputs of sinpi, we have tested that exponent of
    // polynomial[0] is >= expoonent of poly_mul_hi, so therefore it's safe to
    // use add_exact instead of add_exact_safe.
    T poly_mul_add_lo;
    const T poly_mul_add_hi = abacus::internal::add_exact<T>(
        polynomial[0], poly_mul_hi, &poly_mul_add_lo);
    poly_mul_add_lo += poly_mul_lo;

    return poly_mul_add_hi + poly_mul_add_lo;
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct helper<T, abacus_float> {
  static T _(const T x) {
    const abacus_float polynomial[5] = {+3.1415926366204f, -5.1677096710978f,
                                        +2.5500695377459f, -0.59824115267029f,
                                        +0.77558697671848e-1f};

    return abacus::internal::horner_polynomial(x, polynomial);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct helper<T, abacus_double> {
  static T _(const T x) {
    const abacus_double polynomial[9] = {
        3.14159265358979323766,     -5.16771278004996952964,
        2.55016403987729404323,     -0.599264529318741851290,
        0.821458865699517935384e-1, -0.737043047927581778360e-2,
        0.466299691216533729550e-3, -0.219031914477628858710e-4,
        7.69478758985541321889e-7};

    return abacus::internal::horner_polynomial(x, polynomial);
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T sinpi(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  T xAbs = __abacus_fabs(x);

  T xfract = xAbs - __abacus_floor(xAbs);

  const SignedType cond1 = xfract > (T)0.5;
  xfract = __abacus_select(xfract, (T)1.0 - xfract, cond1);

  // Calculate polynomial of 'sin(pi * sqrt(x)) / sqrt(x)' over input range
  // [ 1e-50, (0.5)^2 ]. See sinpi.sollya or sinpidouble maple worksheet.
  const T poly = helper<T>::_(xfract * xfract);
  const T result = __abacus_copysign(xfract * poly, x);

  return __abacus_select(result, -result, abacus::internal::is_odd(xAbs));
}
}  // namespace
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_sinpi(abacus_half x) { return sinpi<>(x); }
abacus_half2 ABACUS_API __abacus_sinpi(abacus_half2 x) { return sinpi<>(x); }
abacus_half3 ABACUS_API __abacus_sinpi(abacus_half3 x) { return sinpi<>(x); }
abacus_half4 ABACUS_API __abacus_sinpi(abacus_half4 x) { return sinpi<>(x); }
abacus_half8 ABACUS_API __abacus_sinpi(abacus_half8 x) { return sinpi<>(x); }
abacus_half16 ABACUS_API __abacus_sinpi(abacus_half16 x) { return sinpi<>(x); }
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_sinpi(abacus_float x) { return sinpi<>(x); }
abacus_float2 ABACUS_API __abacus_sinpi(abacus_float2 x) { return sinpi<>(x); }
abacus_float3 ABACUS_API __abacus_sinpi(abacus_float3 x) { return sinpi<>(x); }
abacus_float4 ABACUS_API __abacus_sinpi(abacus_float4 x) { return sinpi<>(x); }
abacus_float8 ABACUS_API __abacus_sinpi(abacus_float8 x) { return sinpi<>(x); }
abacus_float16 ABACUS_API __abacus_sinpi(abacus_float16 x) {
  return sinpi<>(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_sinpi(abacus_double x) { return sinpi<>(x); }
abacus_double2 ABACUS_API __abacus_sinpi(abacus_double2 x) {
  return sinpi<>(x);
}
abacus_double3 ABACUS_API __abacus_sinpi(abacus_double3 x) {
  return sinpi<>(x);
}
abacus_double4 ABACUS_API __abacus_sinpi(abacus_double4 x) {
  return sinpi<>(x);
}
abacus_double8 ABACUS_API __abacus_sinpi(abacus_double8 x) {
  return sinpi<>(x);
}
abacus_double16 ABACUS_API __abacus_sinpi(abacus_double16 x) {
  return sinpi<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
