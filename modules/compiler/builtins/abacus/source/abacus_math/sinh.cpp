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
/*
  firstly, use the identity sinh(-x) = -sinh(x) to do away with negatives

  sinh is annoyingly imprecise when x is close to 0, so we do some real funky
  manoeuvring such that we can use expm1(x) = exp(x) - 1 for a more precise
  calculation around this pole.

  sinh(x) = (exp(x) - exp(-x)) / 2
          = (exp(x) - exp(-x) + 1 - 1) / 2
          = ((1 - exp(-x)) + (exp(x) - 1)) / 2
          = (((exp(x) - 1) / exp(x)) + (exp(x) - 1)) / 2
          = ((exp(x) - 1) / 2 * exp(x)) + (exp(x) - 1) / 2
          = ((exp(x) - 1) / (2 * exp(x) - 2 + 2)) + (exp(x) - 1) / 2
          = ((exp(x) - 1) / 2 * (exp(x) - 1) + 2) + (exp(x) - 1) / 2
          = (expm1(x) / (2 * expm1(x) + 2)) + expm1(x) / 2
          = expm1(x) * (1 / (2 * expm1(x) + 2) + (1 / 2))
          = expm1(x) * (1 / (2 * expm1(x) + 2) - (1 / 2) + 1)
          = expm1(x) * (((1 / (expm1(x) + 1) - 1) / 2) + 1)

  Fortunately, for 16-bit floats, we can instead employ a simpler algorithm:

  sinh(x) = (exp(x) - exp(-x)) / 2
          = (exp(x) - exp(-x) - 1 + 1) / 2
          = ((exp(x) - 1) - (exp(-x) - 1)) / 2
          = (expm1(x) - expm1(-x)) / 2

  For large x, exp(x) is too imprecise on its own. Instead, we use the identity;

  exp(x + k - k) = exp(x - k) * exp(k)

  Which we can use to scale x down to a value low enough to enable a precise
  calculation like so;

  sinh(x) = 0.5 * (exp(x) - 1 / exp(x))
          = 0.5 * (exp(x - k) * exp(k) - 1 / (exp(x - k) * exp(k))
          = 0.5 * exp(k) * (exp(x - k) - 1 / exp(x - k))

  For 16-bit float, x is large if greater than 11, and we use k = 11.
  For 32-bit float, x is large if greater than 88, and we use k = 45.
  For 64-bit double, x is large if greater than 400, and we use k = 350.
*/
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct helper<T, abacus_half> {
  using SignedType = typename TypeTraits<T>::SignedType;
  static T _(const T x) {
    const T xAbs = __abacus_fabs(x);

    const T ex = __abacus_expm1(xAbs);
    const T ex_neg = __abacus_expm1(-xAbs);
    T ans = __abacus_copysign((ex - ex_neg) * 0.5f16, x);

    // For xAbs >= 11:
    // sinh(x) = (exp(x) - exp(-x)) / 2
    //           (exp(x - 11) * exp(11) - (exp(-x + 11) * exp(-11)))/2
    //           ((exp(x - 11) * exp(11)) / 2) - (exp(-x+1) * exp(-11) / 2))
    //
    // Because the exp(11) constant is so large, and exp(-11) so small the
    // second half of this expression can be treated as zero. So we are left
    // with: sinh(x) = exp(x-11) * exp(11) / 2
    const SignedType large_threshold = xAbs >= 11.0f16;
    const T exp11by2 = 29936.0f16;  // 'exp(11) / 2' rounded to nearest even
    const T large_ans = __abacus_exp(xAbs - 11.0f16) * exp11by2;
    ans = __abacus_select(ans, large_ans, large_threshold);
    return __abacus_copysign(ans, x);
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct helper<T, abacus_float> {
  using SignedType = typename TypeTraits<T>::SignedType;
  static T _(const T x) {
    const T xAbs = __abacus_fabs(x);

    // exp(45) / 2
    const T e45by2 = 1.7467136387965779968e+19f;

    const SignedType scaleCutoff = xAbs < 88.0f;
    const T scale = __abacus_select(e45by2, (T)0.5f, scaleCutoff);
    const T xNew = __abacus_select(xAbs - 45.0f, xAbs, scaleCutoff);

    const T ex = __abacus_expm1(xNew);

    const T ex_rcp = ((T)1.0f / (ex + 1.0f)) - 1.0f;

    const T ans = (scale * ex_rcp + 1.0f) * ex;

    return __abacus_copysign(ans, x);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct helper<T, abacus_double> {
  using SignedType = typename TypeTraits<T>::SignedType;
  static T _(const T x) {
    const T xAbs = __abacus_fabs(x);

    // exp(350) / 2
    const T e350by2 =
        5.0354544351403987991168793147850446737540384576173197479e151;

    const SignedType scaleCutoff = xAbs < 400.0;
    const T scale = __abacus_select(e350by2, (T)0.5, scaleCutoff);
    const T xNew = __abacus_select(xAbs - 350.0, xAbs, scaleCutoff);

    const T ex = __abacus_expm1(xNew);

    const T ex_rcp = ((T)1.0 / (ex + 1.0)) - 1.0;

    const T ans = (scale * ex_rcp + 1.0) * ex;

    return __abacus_copysign(ans, x);
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T sinh(const T x) {
  return helper<T>::_(x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_sinh(abacus_half x) {
  const abacus_half xAbs = __abacus_fabs(x);
  if (xAbs >= 11.0f16) {
    // sinh(x) = (exp(x) - exp(-x)) / 2
    //           (exp(x - 11) * exp(11) - (exp(-x + 11) * exp(-11)))/2
    //           ((exp(x - 11) * exp(11)) / 2) - (exp(-x+1) * exp(-11) / 2))
    //
    // Because the exp(11) constant is so large, and exp(-11) so small the
    // second half of this expression can be treated as zero. So we are left
    // with: sinh(x) = exp(x-11) * exp(11) / 2
    const abacus_half exp11by2 = 29936.0f16;  // 'exp(11) / 2' RTE rounded
    const abacus_half ans = __abacus_exp(xAbs - 11.0f16) * exp11by2;
    return __abacus_copysign(ans, x);
  }

  const abacus_half ex = __abacus_expm1(xAbs);
  const abacus_half ex_neg = __abacus_expm1(-xAbs);
  return __abacus_copysign((ex - ex_neg) * 0.5f16, x);
}
abacus_half2 ABACUS_API __abacus_sinh(abacus_half2 x) { return sinh<>(x); }
abacus_half3 ABACUS_API __abacus_sinh(abacus_half3 x) { return sinh<>(x); }
abacus_half4 ABACUS_API __abacus_sinh(abacus_half4 x) { return sinh<>(x); }
abacus_half8 ABACUS_API __abacus_sinh(abacus_half8 x) { return sinh<>(x); }
abacus_half16 ABACUS_API __abacus_sinh(abacus_half16 x) { return sinh<>(x); }
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_sinh(abacus_float x) { return sinh<>(x); }
abacus_float2 ABACUS_API __abacus_sinh(abacus_float2 x) { return sinh<>(x); }
abacus_float3 ABACUS_API __abacus_sinh(abacus_float3 x) { return sinh<>(x); }
abacus_float4 ABACUS_API __abacus_sinh(abacus_float4 x) { return sinh<>(x); }
abacus_float8 ABACUS_API __abacus_sinh(abacus_float8 x) { return sinh<>(x); }
abacus_float16 ABACUS_API __abacus_sinh(abacus_float16 x) { return sinh<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_sinh(abacus_double x) { return sinh<>(x); }
abacus_double2 ABACUS_API __abacus_sinh(abacus_double2 x) { return sinh<>(x); }
abacus_double3 ABACUS_API __abacus_sinh(abacus_double3 x) { return sinh<>(x); }
abacus_double4 ABACUS_API __abacus_sinh(abacus_double4 x) { return sinh<>(x); }
abacus_double8 ABACUS_API __abacus_sinh(abacus_double8 x) { return sinh<>(x); }
abacus_double16 ABACUS_API __abacus_sinh(abacus_double16 x) {
  return sinh<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
