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

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

namespace {
/*
  firstly, use the identity tanh(-x) = -tanh(x) to do away with negatives

  tanh(x) = sinh(x) / cosh(x)
          = (exp(x) / (exp(-x) + exp(x)))  - (exp(-x) / (exp(-x) + exp(x)))
          = (exp(2x) / (exp(x) * (exp(-x) + exp(x)))) -
            (1 / (exp(x) * (exp(-x) + exp(x)))
          = (exp(2x) / (exp(0) + exp(2x))) - (1 / (exp(0) + exp(2x)))
          = (exp(2x) / (1 + exp(2x))) - (1 / (1 + exp(2x)))
          = exp(2x) * (1 + exp(2x)) - 1
          = (exp(2x) - 1) / (exp(2x) + 1)
          = expm1(2x) / (exp(2x) + 1)
          = expm1(2x) / (exp(2x) + 1)
          = expm1(2x) / (expm1(2x) + 2)
*/

template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T> struct helper<T, abacus_half> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;
    typedef typename TypeTraits<T>::UnsignedType UnsignedType;

    const T xAbs = __abacus_fabs(x);

    T ex = __abacus_expm1(xAbs * 2.0f16);
    const T divide = ex / (ex + 2.0f16);

    const T next_after = abacus::detail::cast::as<T>(UnsignedType(
        abacus::detail::cast::as<UnsignedType>(divide) + UnsignedType(1)));

    // Within these bounds we need to use nextafter() otherwise we're between 2
    // to 3 ulp out.
    //
    // The bounds are:
    //   0x27F3 <= |x| <= 0x27F5
    //   0x2BEE <= |x| <= 0x2BF0
    //   |x| == 0x2AD1
    //   |x| == 0x2F88
    SignedType cond_next =
        ((xAbs >= 0.0310516357421875f16) && (xAbs <= 0.0310821533203125f16)) ||
        ((xAbs >= 0.06195068359375f16) && (xAbs <= 0.06201171875f16)) ||
        (xAbs == 0.053253173828125f16) || (xAbs == 0.11767578125f16);
    ex = __abacus_select(divide, next_after, cond_next);

    ex = __abacus_copysign(ex, x);

    // Input which results in an output of the smallest value less than 1
    const SignedType cond1 = xAbs > 4.15625f16;
    ex = __abacus_select(ex, __abacus_copysign(1.0f16, x), cond1);

    const SignedType cond2 = (xAbs == 0.0f16) | __abacus_isnan(x);
    return __abacus_select(ex, x, cond2);
  }
};
#endif // __CA_BUILTINS_HALF_SUPPORT

template <typename T> struct helper<T, abacus_float> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::UnsignedType UnsignedType;

    T ex = __abacus_expm1(x * 2.0f);
    ex = abacus::detail::cast::as<T>(
        abacus::detail::cast::as<UnsignedType>(ex / (ex + 2.0f)) + 1);

    const T xAbs = __abacus_fabs(x);

    ex = __abacus_select(ex, __abacus_copysign(1.0f, x), xAbs > 8.0f);
    return __abacus_select(ex, x, (xAbs == 0.0f) | __abacus_isnan(x));
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T> struct helper<T, abacus_double> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    T ex = __abacus_expm1(x * 2.0);
    ex = ex / (ex + 2.0);

    const T xAbs = __abacus_fabs(x);

    const SignedType cond1 = xAbs > 18.0;
    ex = __abacus_select(ex, __abacus_copysign(1.0, x), cond1);

    const SignedType cond2 = (xAbs == 0.0) | __abacus_isnan(x);
    return __abacus_select(ex, x, cond2);
  }
};
#endif // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T> T tanh(const T x) { return helper<T>::_(x); }
} // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_tanh(abacus_half x) { return tanh<>(x); }
abacus_half2 ABACUS_API __abacus_tanh(abacus_half2 x) { return tanh<>(x); }
abacus_half3 ABACUS_API __abacus_tanh(abacus_half3 x) { return tanh<>(x); }
abacus_half4 ABACUS_API __abacus_tanh(abacus_half4 x) { return tanh<>(x); }
abacus_half8 ABACUS_API __abacus_tanh(abacus_half8 x) { return tanh<>(x); }
abacus_half16 ABACUS_API __abacus_tanh(abacus_half16 x) { return tanh<>(x); }
#endif // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_tanh(abacus_float x) { return tanh<>(x); }
abacus_float2 ABACUS_API __abacus_tanh(abacus_float2 x) { return tanh<>(x); }
abacus_float3 ABACUS_API __abacus_tanh(abacus_float3 x) { return tanh<>(x); }
abacus_float4 ABACUS_API __abacus_tanh(abacus_float4 x) { return tanh<>(x); }
abacus_float8 ABACUS_API __abacus_tanh(abacus_float8 x) { return tanh<>(x); }
abacus_float16 ABACUS_API __abacus_tanh(abacus_float16 x) { return tanh<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_tanh(abacus_double x) { return tanh<>(x); }
abacus_double2 ABACUS_API __abacus_tanh(abacus_double2 x) { return tanh<>(x); }
abacus_double3 ABACUS_API __abacus_tanh(abacus_double3 x) { return tanh<>(x); }
abacus_double4 ABACUS_API __abacus_tanh(abacus_double4 x) { return tanh<>(x); }
abacus_double8 ABACUS_API __abacus_tanh(abacus_double8 x) { return tanh<>(x); }
abacus_double16 ABACUS_API __abacus_tanh(abacus_double16 x) {
  return tanh<>(x);
}
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
