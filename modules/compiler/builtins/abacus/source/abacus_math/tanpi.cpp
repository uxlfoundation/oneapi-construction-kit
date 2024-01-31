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
#include <abacus/internal/horner_polynomial.h>
#include <abacus/internal/is_odd.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct helper<T, abacus_half> {
  static T numerator(const T x) {
    // Approximation of sin(pi * sqrt(x)) / sqrt(x)
    // See tanpi.sollya

    // Unfortunately, 4 polynomial terms at 16 bits is not quite enough
    // precision. However, to avoid adding any more polynomial terms, one trick
    // we can use here is to make the last term extra precise. So instead of
    // your normal 16bit polynomial a + b*x + c*x^2 + d*x^3 + .. with a,b,c,d
    // all 16 bit, we instead let 'a' have 22 bits of precision instead of the
    // normal 11 for half. This has the nice property that the constant 'a' can
    // now be split into the sum of 2 halfs: a_hi and a_lo.
    //
    // So now our polynomial is:   a_lo + (a_hi + b*x + c*x^2 + d*x^3 +....),
    //
    // Note that in this particular implementation, the result was more accurate
    // after swapping a_hi and a_lo.
    const abacus_half polynomial[4] = {0.00096893310546875f16, -5.16796875f16,
                                       2.55859375f16, -0.66943359375f16};
    const abacus_half polynomial_0_hi = 3.140625f16;
    return x * (polynomial_0_hi +
                abacus::internal::horner_polynomial(x * x, polynomial));
  }

  static T denominator(const T x) {
    // Approximation of cos(pi * sqrt(x))
    // See tanpi.sollya
    const abacus_half polynomial[4] = {1.0f16, -4.93359375f16, 4.00390625f16,
                                       -0.74072265625f16};
    return abacus::internal::horner_polynomial(x * x, polynomial);
  }

  static T handle_edge_cases(const T xAbs, const T ans) {
    // The current algorithm used by the `numerator` function above results in a
    // calculation for tanpi(x) which is within 2 ULP of the reference function,
    // except for a single case.
    //
    // The only way to fix that single case was to make use of `multiply_exact`
    // and `add_exact` in the return expression, adding dozens of extra FP16
    // operations. Instead of adding these extra calculations, we handle that
    // case explicitly here.
    using SignedType = typename TypeTraits<T>::SignedType;
    return __abacus_select(ans, T(0.978027f16), SignedType(xAbs == 0.24646f16));
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct helper<T, abacus_float> {
  static T numerator(const T x) {
    return x * 3.14159260961f - x * x * x * 2.97043292307f;
  }

  static T denominator(const T x) {
    return (T)1.f - x * x * 4.23539290179f + x * x * x * x * 0.946484572927f;
  }

  static T handle_edge_cases(const T, const T ans) { return ans; }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct helper<T, abacus_double> {
  static T numerator(const T x) {
    const abacus_double polynomial[4] = {
        3.6490197133941196023, -4.6200497777346237839, 0.99735716184355045101,
        -0.26253549797567171127e-1};

    return x * abacus::internal::horner_polynomial(x * x, polynomial);
  }

  static T denominator(const T x) {
    const abacus_double polynomial[4] = {
        1.1615190496528906454, -5.2918520270485559140, 2.6412953506383059363,
        -0.23276806353523888909};

    return abacus::internal::horner_polynomial(x * x, polynomial);
  }

  static T handle_edge_cases(const T, const T ans) { return ans; }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T tanpi(const T x) {
  using SignedType = typename TypeTraits<T>::SignedType;

  const T xAbs = __abacus_fabs(x);

  T xfract = xAbs - __abacus_floor(xAbs);

  const SignedType cond1 = xfract > T(0.25);
  const SignedType cond2 = xfract < T(0.75);

  const T xfract_part = __abacus_select(T(1.0), T(0.5), cond2);

  // Put x in polynomial range [0, 0.25]
  xfract = __abacus_select(xfract, xfract - xfract_part, cond1);

  const T top = helper<T>::numerator(xfract);       // Approximation of sinpi()
  const T bottom = helper<T>::denominator(xfract);  // Approximation of cospi()

  // Check if we want the cotangent -cos(x) / sin(x)
  const SignedType use_cotan = cond1 & cond2;

  T ans = __abacus_select(top, -bottom, use_cotan) /
          __abacus_select(bottom, top, use_cotan);

  // If the answer is ABACUS_INFINITY we need to check which sign it should be
  // (at this points its always negative inf)
  const SignedType cond3 = abacus::internal::is_odd(x);
  const T ans_part =
      __abacus_select(T(ABACUS_INFINITY), T(-ABACUS_INFINITY), cond3);

  const SignedType cond4 = __abacus_isinf(ans);
  ans = __abacus_select(ans, ans_part, cond4);

  // Handle any edge cases before negating the answer for negative values of x.
  ans = helper<T>::handle_edge_cases(xAbs, ans);

  const SignedType cond5 = x < T(0.0);
  return __abacus_select(ans, -ans, cond5);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_tanpi(abacus_half x) { return tanpi<>(x); }
abacus_half2 ABACUS_API __abacus_tanpi(abacus_half2 x) { return tanpi<>(x); }
abacus_half3 ABACUS_API __abacus_tanpi(abacus_half3 x) { return tanpi<>(x); }
abacus_half4 ABACUS_API __abacus_tanpi(abacus_half4 x) { return tanpi<>(x); }
abacus_half8 ABACUS_API __abacus_tanpi(abacus_half8 x) { return tanpi<>(x); }
abacus_half16 ABACUS_API __abacus_tanpi(abacus_half16 x) { return tanpi<>(x); }
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_tanpi(abacus_float x) { return tanpi<>(x); }
abacus_float2 ABACUS_API __abacus_tanpi(abacus_float2 x) { return tanpi<>(x); }
abacus_float3 ABACUS_API __abacus_tanpi(abacus_float3 x) { return tanpi<>(x); }
abacus_float4 ABACUS_API __abacus_tanpi(abacus_float4 x) { return tanpi<>(x); }
abacus_float8 ABACUS_API __abacus_tanpi(abacus_float8 x) { return tanpi<>(x); }
abacus_float16 ABACUS_API __abacus_tanpi(abacus_float16 x) {
  return tanpi<>(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_tanpi(abacus_double x) { return tanpi<>(x); }
abacus_double2 ABACUS_API __abacus_tanpi(abacus_double2 x) {
  return tanpi<>(x);
}
abacus_double3 ABACUS_API __abacus_tanpi(abacus_double3 x) {
  return tanpi<>(x);
}
abacus_double4 ABACUS_API __abacus_tanpi(abacus_double4 x) {
  return tanpi<>(x);
}
abacus_double8 ABACUS_API __abacus_tanpi(abacus_double8 x) {
  return tanpi<>(x);
}
abacus_double16 ABACUS_API __abacus_tanpi(abacus_double16 x) {
  return tanpi<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
