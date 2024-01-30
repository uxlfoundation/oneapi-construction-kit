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
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/horner_polynomial.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
// Polynomial approximating the function log(x+1)/x over the
// range [sqrt(0.5)-1, sqrt(2)-1]. See log.sollya for derivation
static ABACUS_CONSTANT abacus_half __codeplay_log_coeff_half[9] = {
    1.0f16,
    -0.5f16,
    0.333251953125f16,
    -0.25f16,
    0.202392578125f16,
    -0.1673583984375f16,
    0.1256103515625f16,
    -0.1241455078125f16,
    0.12432861328125f16};

template <>
struct helper<abacus_half, abacus_half> {
  static abacus_half _(const abacus_half x) {
    // Check for special cases: +/-ABACUS_INFINITY, ABACUS_NAN, negative numbers
    if ((__abacus_isnan(x) || x < -0.0f16)) {
      return ABACUS_NAN_H;
    }

    if (__abacus_isinf(x) || x == -0.0f16) {
      return (x == 0.0f16) ? -ABACUS_INFINITY : ABACUS_INFINITY;
    }

    abacus_int exponent = 0;
    abacus_half significand = __abacus_frexp(x, &exponent);

    // Scale the significand in order to fit in the domain of the polynomial
    // approximation
    if (significand < abacus_half(ABACUS_SQRT1_2_F)) {
      significand *= 2.0f16;
      exponent--;
    }

    // We are using the polynomial approximation of x+1 so we need to reduce
    // input by one.
    significand = significand - 1.0f16;

    const abacus_half poly_approx = abacus::internal::horner_polynomial(
        significand, __codeplay_log_coeff_half);

    const abacus_half result = significand * poly_approx;
    const abacus_half fexponent(exponent);
    return result + (fexponent * abacus_half(ABACUS_LN2_F));
  }
};

// Vectorized version:
template <typename T>
struct helper<T, abacus_half> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    typedef typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type
        IntVecType;

    IntVecType exponent = 0;
    T significand = __abacus_frexp(x, &exponent);

    SignedType exponent_short =
        abacus::detail::cast::convert<SignedType>(exponent);
    // Scale the significand in order to fit in the domain of the polynomial
    // approximation
    SignedType cond = significand < T(ABACUS_SQRT1_2_F);

    significand = __abacus_select(significand, significand * 2.0f16, cond);
    exponent_short = __abacus_select(exponent_short, exponent_short - 1, cond);

    // We are using the polynomial approximation of x+1 so we need to reduce
    // input by one.
    significand = significand - 1.0f16;

    T result = significand * abacus::internal::horner_polynomial(
                                 significand, __codeplay_log_coeff_half);

    result = result + (abacus::detail::cast::convert<T>(exponent_short) *
                       T(ABACUS_LN2_F));

    result = __abacus_select(
        result,
        __abacus_select(T(ABACUS_INFINITY), T(-ABACUS_INFINITY), x == 0.0f16),
        (x == 0.0f16) | __abacus_isinf(x));

    result = __abacus_select(result, FPShape<T>::NaN(),
                             (x < -0.0f16) | __abacus_isnan(x));  // nan

    return result;
  }
};
#endif  //__CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct helper<T, abacus_float> {
  static T _(const T x) { return __abacus_log2(x) * ABACUS_LN2_F; }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct helper<T, abacus_double> {
  static T _(const T x) { return __abacus_log2(x) * ABACUS_LN2; }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T ABACUS_API log(const T x) {
  return helper<T>::_(x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_log(abacus_half x) { return log<>(x); }
abacus_half2 ABACUS_API __abacus_log(abacus_half2 x) { return log<>(x); }
abacus_half3 ABACUS_API __abacus_log(abacus_half3 x) { return log<>(x); }
abacus_half4 ABACUS_API __abacus_log(abacus_half4 x) { return log<>(x); }
abacus_half8 ABACUS_API __abacus_log(abacus_half8 x) { return log<>(x); }
abacus_half16 ABACUS_API __abacus_log(abacus_half16 x) { return log<>(x); }
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_log(abacus_float x) { return log<>(x); }
abacus_float2 ABACUS_API __abacus_log(abacus_float2 x) { return log<>(x); }
abacus_float3 ABACUS_API __abacus_log(abacus_float3 x) { return log<>(x); }
abacus_float4 ABACUS_API __abacus_log(abacus_float4 x) { return log<>(x); }
abacus_float8 ABACUS_API __abacus_log(abacus_float8 x) { return log<>(x); }
abacus_float16 ABACUS_API __abacus_log(abacus_float16 x) { return log<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_log(abacus_double x) { return log<>(x); }
abacus_double2 ABACUS_API __abacus_log(abacus_double2 x) { return log<>(x); }
abacus_double3 ABACUS_API __abacus_log(abacus_double3 x) { return log<>(x); }
abacus_double4 ABACUS_API __abacus_log(abacus_double4 x) { return log<>(x); }
abacus_double8 ABACUS_API __abacus_log(abacus_double8 x) { return log<>(x); }
abacus_double16 ABACUS_API __abacus_log(abacus_double16 x) { return log<>(x); }
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
