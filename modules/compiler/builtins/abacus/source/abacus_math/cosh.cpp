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
#include <abacus/abacus_type_traits.h>

#include <abacus/internal/exp_unsafe.h>

namespace {
/*
  firstly, use the identity cosh(-x) = cosh(x) to do away with negatives

  cosh(x) = (exp(x) + exp(-x)) / 2
          = (exp(2x) + 1) / (2 * exp(x))
          = (exp(2x) / exp(x) + 1 / exp(x)) / 2
          = (exp(2x) * exp(-x) + 1 / exp(x)) / 2
          = (exp(2x - x) + 1 / exp(x)) / 2
          = (exp(x) + 1 / exp(x)) / 2
          = 0.5 * (exp(x) + 1 / exp(x))

  for large x, exp(x) is too imprecise on its own. Instead, we use the identity;

  exp(x + k - k) = exp(x - k) * exp(k)

  which we can use to scale x down to a value low enough to enable a precise
  calculation like so;

  cosh(x) = 0.5 * (exp(x) + 1 / exp(x))
          = 0.5 * (exp(x - k) * exp(k) + 1 / (exp(x - k) * exp(k))
          = 0.5 * exp(k) * (exp(x - k) + 1 / exp(x - k))

  For 16-bit float, x is large if greater than 10, and we use k = 5.
  For 32-bit float, x is large if greater than 70, and we use k = 45.
  For 64-bit double, x is large if greater than 400, and we use k = 350.
*/
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct helper<T, abacus_half> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    const T xAbs = __abacus_fabs(x);

    // exp(5) / 2
    const T e5by2 = 74.1875f16;

    const SignedType scaleCutoff = xAbs < 10.0f16;
    const T scale = __abacus_select(e5by2, 0.5f16, scaleCutoff);

    const T reduced = __abacus_select(T(xAbs - 5.0f16), xAbs, scaleCutoff);

    const T ex = __abacus_exp(reduced);

    const T ans = (ex + (T(1.0) / ex)) * scale;

    // check largest valid input value before output overflows
    const SignedType infCutoff = xAbs > 11.78125f16;
    return __abacus_select(ans, T(ABACUS_INFINITY), infCutoff);
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct helper<T, abacus_float> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    const T xAbs = __abacus_fabs(x);

    // exp(45) / 2
    const T e45by2 = 1.7467136387965779968e+19f;

    const SignedType scaleCutoff = xAbs < 70.0f;
    const T scale = __abacus_select(e45by2, (T)0.5f, scaleCutoff);

    const T reduced = __abacus_select(xAbs - 45.0f, xAbs, scaleCutoff);

    const T ex = abacus::internal::exp_unsafe(reduced);

    const T ans = (ex + ((T)1.0f / ex)) * scale;

    const SignedType infCutoff = xAbs > 89.415985107421875f;
    return __abacus_select(ans, ABACUS_INFINITY, infCutoff);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct helper<T, abacus_double> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    const T xAbs = __abacus_fabs(x);

    // exp(350) / 2
    const T e350by2 =
        5.0354544351403987991168793147850446737540384576173197479e151;

    const SignedType scaleCutoff = xAbs < 400.0;
    const T scale = __abacus_select(e350by2, (T)0.5, scaleCutoff);

    const T reduced = __abacus_select(xAbs - 350.0, xAbs, scaleCutoff);

    const T ex = __abacus_exp(reduced);

    const T ans = (ex + ((T)1.0 / ex)) * scale;

    const SignedType infCutoff = xAbs > 710.475860073943942041640622032117;
    return __abacus_select(ans, (T)ABACUS_INFINITY, infCutoff);
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T cosh(const T x) {
  return helper<T>::_(x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_cosh(abacus_half x) { return cosh<>(x); }
abacus_half2 ABACUS_API __abacus_cosh(abacus_half2 x) { return cosh<>(x); }
abacus_half3 ABACUS_API __abacus_cosh(abacus_half3 x) { return cosh<>(x); }
abacus_half4 ABACUS_API __abacus_cosh(abacus_half4 x) { return cosh<>(x); }
abacus_half8 ABACUS_API __abacus_cosh(abacus_half8 x) { return cosh<>(x); }
abacus_half16 ABACUS_API __abacus_cosh(abacus_half16 x) { return cosh<>(x); }
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_cosh(abacus_float x) { return cosh<>(x); }
abacus_float2 ABACUS_API __abacus_cosh(abacus_float2 x) { return cosh<>(x); }
abacus_float3 ABACUS_API __abacus_cosh(abacus_float3 x) { return cosh<>(x); }
abacus_float4 ABACUS_API __abacus_cosh(abacus_float4 x) { return cosh<>(x); }
abacus_float8 ABACUS_API __abacus_cosh(abacus_float8 x) { return cosh<>(x); }
abacus_float16 ABACUS_API __abacus_cosh(abacus_float16 x) { return cosh<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_cosh(abacus_double x) { return cosh<>(x); }
abacus_double2 ABACUS_API __abacus_cosh(abacus_double2 x) { return cosh<>(x); }
abacus_double3 ABACUS_API __abacus_cosh(abacus_double3 x) { return cosh<>(x); }
abacus_double4 ABACUS_API __abacus_cosh(abacus_double4 x) { return cosh<>(x); }
abacus_double8 ABACUS_API __abacus_cosh(abacus_double8 x) { return cosh<>(x); }
abacus_double16 ABACUS_API __abacus_cosh(abacus_double16 x) {
  return cosh<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
