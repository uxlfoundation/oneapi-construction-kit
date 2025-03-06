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

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/is_denorm.h>
#include <abacus/internal/ldexp_unsafe.h>
#include <abacus/internal/rsqrt_unsafe.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct helper<T, abacus_half> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;
    typedef typename TypeTraits<T>::UnsignedType UnsignedType;

    // To prevent intermediate underflow/overflow in the rsqrt_unsafe
    // function if x is below a certain threshold value (0x0800) we
    // scale by 2^10 at the start to bring it into acceptable bounds,
    // and by 2^5 at the end to rescale the answer back down
    SignedType xSmall =
        abacus::detail::cast::as<UnsignedType>(x) < UnsignedType(0x0800);
    T processedX = __abacus_select(x, x * 1024.0f16, xSmall);  // 2^10

    // estimate rsqrt
    T estimate = abacus::internal::rsqrt_unsafe(processedX);

    T result = __abacus_select(estimate, estimate * 32.0f16, xSmall);

    // Infinity and 0 check:
    // Nice way of getting the correct return values for -0.0, 0.0, and
    // INFINITY:
    T inf_or_zero_return = abacus::detail::cast::as<T>(
        UnsignedType((abacus::detail::cast::as<UnsignedType>(x) ^
                      UnsignedType(FPShape<abacus_half>::ExponentMask()))));
    result = __abacus_select(result, inf_or_zero_return,
                             SignedType((__abacus_isinf(x) || (x == 0.0f16))));
    // Nan checks
    result =
        __abacus_select(result, FPShape<T>::NaN(),
                        abacus::detail::cast::convert<SignedType>(x < 0.0f16));

    return result;
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct helper<T, abacus_float> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;
    typedef typename TypeTraits<T>::UnsignedType UnsignedType;

    const UnsignedType xUint = abacus::detail::cast::as<UnsignedType>(x);

    // We use the exact bounds for rtz as it also works with ftz and the
    // other rounding modes.
    const SignedType xBig = (xUint >= 0x7e6eb3c0);

    // Denormal number, i.e exponent bits are zero and implicit leading 1 is
    // dropped.
    const SignedType xSmall = abacus::internal::is_denorm(x);

    const UnsignedType hiddenBit = 0x00800000u;  // Lowest exponent bit

    // Scale denormal to improve fast rqsrt starting newton-raphson value.
    // Getting ldexp(x, 24) to normalise x without an ldexp call, since we know
    // x is denorm and the result is normal.
    //
    // xUint | hiddenBit      Gives an exponent of -126
    // 16777216               2^24
    // Multiplication         exponent = -126 + 24 = -102
    //
    // 0x0C800000             2^(-102)
    // processedX             (x * 2^24) + 2^-102 - 2^-102
    //                        (x * 2^24)
    T processedX = __abacus_select(
        x,
        (abacus::detail::cast::as<T>(xUint | hiddenBit) * 16777216.0f) -
            __abacus_as_float(0x0C800000),
        xSmall);

    // 0.0625                 2^-4
    // processedX             Scale exponent down towards zero by 4
    processedX = __abacus_select(processedX, x * 0.0625f, xBig);

    // Use fast rsqrt algorithm from Quake.
    // [https://en.wikipedia.org/wiki/Fast_inverse_square_root]
    T ans = abacus::internal::rsqrt_unsafe(processedX);

    // Scales answer back up since we decreased the magnitude in processedX.
    // As the rsqrt of a denormal will have a positive exponent we multiply by
    // 4096, chosen since it's sqrt(116777216) from processedX initialisation.
    ans = __abacus_select(ans, ans * 4096.0f, xSmall);

    // Since we decreased the magnitude creating processsedX for xBig, bump it
    // back up by multiplying by sqrt(0.0625) => 0.25
    // We want the result to be smaller since we have a negative exponent.
    ans = __abacus_select(ans, ans * 0.25f, xBig);

    ans = __abacus_select(ans, 0.0f, __abacus_isinf(x));

    ans =
        __abacus_select(ans, FPShape<T>::NaN(), (x < 0.0f) | __abacus_isnan(x));

    ans = __abacus_select(ans, __abacus_copysign(ABACUS_INFINITY, x),
                          __abacus_fabs(x) == 0.0f);

    return ans;
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct helper<T, abacus_double> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type xExp;
    T xMant = __abacus_frexp(x, &xExp);

    const SignedType expOddCond =
        abacus::detail::cast::convert<SignedType>((xExp & 0x1) == 0x1);
    xMant = __abacus_select(xMant, xMant * 2.0, expOddCond);

    // estimate rsqrt from 0.5 -> 2
    const T estimate = abacus::internal::rsqrt_unsafe(xMant);

    T result = abacus::internal::ldexp_unsafe(estimate, -(xExp >> 1));

    const SignedType cond1 = x == 0.0;
    result = __abacus_select(result, __abacus_copysign((T)ABACUS_INFINITY, x),
                             cond1);

    const SignedType cond2 = __abacus_isinf(x);
    result = __abacus_select(result, (T)0.0, cond2);

    const SignedType cond3 = x < 0.0;
    result = __abacus_select(result, (T)ABACUS_NAN, cond3);

    return result;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T rsqrt(const T x) {
  return helper<T>::_(x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT

abacus_half ABACUS_API __abacus_rsqrt(abacus_half x) { return rsqrt<>(x); }
abacus_half2 ABACUS_API __abacus_rsqrt(abacus_half2 x) { return rsqrt<>(x); }
abacus_half3 ABACUS_API __abacus_rsqrt(abacus_half3 x) { return rsqrt<>(x); }
abacus_half4 ABACUS_API __abacus_rsqrt(abacus_half4 x) { return rsqrt<>(x); }
abacus_half8 ABACUS_API __abacus_rsqrt(abacus_half8 x) { return rsqrt<>(x); }
abacus_half16 ABACUS_API __abacus_rsqrt(abacus_half16 x) { return rsqrt<>(x); }

#endif  //__CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_rsqrt(abacus_float x) { return rsqrt<>(x); }
abacus_float2 ABACUS_API __abacus_rsqrt(abacus_float2 x) { return rsqrt<>(x); }
abacus_float3 ABACUS_API __abacus_rsqrt(abacus_float3 x) { return rsqrt<>(x); }
abacus_float4 ABACUS_API __abacus_rsqrt(abacus_float4 x) { return rsqrt<>(x); }
abacus_float8 ABACUS_API __abacus_rsqrt(abacus_float8 x) { return rsqrt<>(x); }
abacus_float16 ABACUS_API __abacus_rsqrt(abacus_float16 x) {
  return rsqrt<>(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_rsqrt(abacus_double x) { return rsqrt<>(x); }
abacus_double2 ABACUS_API __abacus_rsqrt(abacus_double2 x) {
  return rsqrt<>(x);
}
abacus_double3 ABACUS_API __abacus_rsqrt(abacus_double3 x) {
  return rsqrt<>(x);
}
abacus_double4 ABACUS_API __abacus_rsqrt(abacus_double4 x) {
  return rsqrt<>(x);
}
abacus_double8 ABACUS_API __abacus_rsqrt(abacus_double8 x) {
  return rsqrt<>(x);
}
abacus_double16 ABACUS_API __abacus_rsqrt(abacus_double16 x) {
  return rsqrt<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
