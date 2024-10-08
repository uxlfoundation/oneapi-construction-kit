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
#include <abacus/internal/floor_unsafe.h>
#include <abacus/internal/horner_polynomial.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct helper<T, abacus_half> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;
    typedef typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type
        IntType;

    // Range reduction algorithms try to find an integer 'k', and constant
    // 'C' such that y = x - kC, where y is in [0,C] or [-C/2, +C/2]. We can
    // use y over the reduced range of a polynomial instead of original input x.
    //
    // For exp10 C is 'log(2) / log(10)'.
    //
    // Cody & Waite represents C by the addition of two floating point numbers
    // for accuracy, giving y = (x - k*C1) - k*C2
    //
    // Here C1 & C2 are 'codyWaite1' and 'codyWaite2'.

    // 1 / log10(2.0) is 3.32192, which falls in-between the adjacent half
    // values 3.32031 & 3.32227. Go for 3.32227 since it's closer.
    const T log10_2rcp = 3.32227f16;

    // Find k for range reduction, 0.5 is just to force k into the
    // range [-0.5, 0.5]
    const T cody = x * log10_2rcp + 0.5f16;
    const SignedType k = abacus::internal::floor_unsafe(cody);
    const T kf = abacus::detail::cast::convert<T>(k);

    // These C1 & C2 values are slightly different from float & double, but
    // still sum to the same C. C1 was made smaller so that the last precision
    // bit of the mantissa isn't set, making k*C1 more accurate. This means C2
    // can be made larger, causing it to no longer be a denormal value which
    // it would be in half precision when using the same C2 from float & double.

    const T codyWaite1 = 0.300781f16;
    const T codyWaite2 = 0.0002489956640664981f16;

    // Range reduced x
    const T rr_x = (x - (kf * codyWaite1)) - (kf * codyWaite2);

    // See exp10.sollya for derivation of polynomial coefficients
    const abacus_half polynomial[5] = {1.0f16, 2.302734375f16, 2.65234375f16,
                                       2.037109375f16, 1.1005859375f16};

    // Minimax polynomial approximation in the domain
    // [-log(2)/(2*log(10)), log(2)/(2*log(10))]
    const T result = abacus::internal::horner_polynomial(rr_x, polynomial);

    const IntType kInt = abacus::detail::cast::convert<IntType>(k);
    const T raise = __abacus_ldexp(result, kInt);

    // Check if x is less than the power which results in smallest denormal
    // Half smallest denormal is 2^-24, and log10(2^-24) ==> -7.224719895935548
    const SignedType cond = x < -7.224719895935548f16;
    return __abacus_select(raise, 0.0f16, cond);
  }
};
#endif

template <typename T>
struct helper<T, abacus_float> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    // Cody & Waite range reduction
    const T codyWaite1 = 0.301025390625f;
    const T codyWaite2 = 4.60503906651865690946578979492E-6f;
    const T log10_2rcp =
        3.3219280948873623478703194294893901758648313930245806f;

    // 0.5f is just to force k into the range [-0.5f, 0.5f]
    const SignedType k =
        (abacus::internal::floor_unsafe((x * log10_2rcp) + 0.5f));
    const T kf = abacus::detail::cast::convert<T>(k);
    const T r = (x - (kf * codyWaite1)) - (kf * codyWaite2);

    const abacus_float polynomial[7] = {1.00000000055421f, 2.30258517662976f,
                                        2.65094863530153f, 2.03464849984362f,
                                        1.17129897116617f, 0.542067923168435f,
                                        0.206220193305040f};

    // Minimax polynomial approximation in the domain [-log10(2)/2, log10(2)/2]
    const T result = abacus::internal::horner_polynomial(r, polynomial);

    return __abacus_select(__abacus_ldexp(result, k), 0.0f,
                           x < -44.8534698486328125f);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct helper<T, abacus_double> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    const T codyWaite1 = __abacus_as_double(
        (abacus_long)0x3FD34413509F7800);  // 3.0102999566395283181918784976e-1;
    const T codyWaite2 = __abacus_as_double((
        abacus_long)0x3D1FEF311F12B000);  // 2.8363394551042263094578877949e-14;
    const T codyWaite3 =
        2.7013429058980533685465482739654553714956252304807843727681772521181861721e-27;
    const T log10_2rcp = 3.3219280948873623478703194294893901758648313930246;

    const SignedType k = abacus::internal::floor_unsafe(x * log10_2rcp);
    const T kf = abacus::detail::cast::convert<T>(k);
    const T r =
        ((x - (kf * codyWaite1)) - (kf * codyWaite2)) - (kf * codyWaite3);

    const abacus_double polynomial[13] = {
        0.1000000000000000000040723e1,  0.2302585092994045636094307e1,
        0.2650949055239208283331811e1,  0.2034678592292774676519519e1,
        0.1171255148939683427837687e1,  0.5393829285608707357040927e0,
        0.2069958580877315045292485e0,  0.6808927237335015024118666e-1,
        0.1959831805361454777673764e-1, 0.5011066479908280225233138e-2,
        0.1163303810486714099444940e-2, 0.2244268226650114145644564e-3,
        0.6543870973084372896203514e-4};

    // minimax from 0 -> ln(10)/ln(2) of 10^x
    const T fract = abacus::internal::horner_polynomial(r, polynomial);

    // The answer is ldexp(poly, quotient), but we don't need the full ldexp
    const T factor1 = abacus::detail::cast::as<T>((k / 2 + 1023) << 52);
    const T factor2 = abacus::detail::cast::as<T>((k - (k / 2) + 1023) << 52);

    T result = fract * factor1 * factor2;

    const SignedType cond1 = x < -323.306215343;
    result = __abacus_select(result, (T)0.0, cond1);

    const SignedType cond2 = x > 308.25471556;
    result = __abacus_select(result, (T)ABACUS_INFINITY, cond2);

    return result;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T exp10(const T x) {
  return helper<T>::_(x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_exp10(abacus_half x) { return exp10<>(x); }
abacus_half2 ABACUS_API __abacus_exp10(abacus_half2 x) { return exp10<>(x); }
abacus_half3 ABACUS_API __abacus_exp10(abacus_half3 x) { return exp10<>(x); }
abacus_half4 ABACUS_API __abacus_exp10(abacus_half4 x) { return exp10<>(x); }
abacus_half8 ABACUS_API __abacus_exp10(abacus_half8 x) { return exp10<>(x); }
abacus_half16 ABACUS_API __abacus_exp10(abacus_half16 x) { return exp10<>(x); }
#endif

abacus_float ABACUS_API __abacus_exp10(abacus_float x) { return exp10<>(x); }
abacus_float2 ABACUS_API __abacus_exp10(abacus_float2 x) { return exp10<>(x); }
abacus_float3 ABACUS_API __abacus_exp10(abacus_float3 x) { return exp10<>(x); }
abacus_float4 ABACUS_API __abacus_exp10(abacus_float4 x) { return exp10<>(x); }
abacus_float8 ABACUS_API __abacus_exp10(abacus_float8 x) { return exp10<>(x); }
abacus_float16 ABACUS_API __abacus_exp10(abacus_float16 x) {
  return exp10<>(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_exp10(abacus_double x) { return exp10<>(x); }
abacus_double2 ABACUS_API __abacus_exp10(abacus_double2 x) {
  return exp10<>(x);
}
abacus_double3 ABACUS_API __abacus_exp10(abacus_double3 x) {
  return exp10<>(x);
}
abacus_double4 ABACUS_API __abacus_exp10(abacus_double4 x) {
  return exp10<>(x);
}
abacus_double8 ABACUS_API __abacus_exp10(abacus_double8 x) {
  return exp10<>(x);
}
abacus_double16 ABACUS_API __abacus_exp10(abacus_double16 x) {
  return exp10<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
