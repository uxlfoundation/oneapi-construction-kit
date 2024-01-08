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
#include <abacus/abacus_integer.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_type_traits.h>
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

    // Split x into it's integer and remainder parts which we can operate on
    // separately
    const IntType floor = abacus::detail::cast::convert<IntType>(
        abacus::internal::floor_unsafe(x));
    const T remainder = x - abacus::detail::cast::convert<T>(floor);

    // Minimax polynomial approximation in the domain [0.0, 1.0]
    // See exp2.sollya
    const abacus_half polynomial[4] = {1.0f16, 0.6953125f16, 0.2271728515625f16,
                                       7.733154296875e-2f16};

    // We know remainder will be in the reduced range [0.0, 1.0], so we use a
    // polynomial approximation to calculate 2^remainder
    const T fract = abacus::internal::horner_polynomial(remainder, polynomial);

    // Multiply 2^floor by approximation of 2^remainder to get the final result
    T result = __abacus_ldexp(fract, floor);

    // Half precision exponents can't represent values of 16 or over after
    // subtracting bias.
    const T max_cutoff = 16.0f16;
    const SignedType cond1 = x > max_cutoff;
    const T half_infinity = abacus::detail::cast::as<T>((SignedType)0x7C00);
    result = __abacus_select(result, half_infinity, cond1);

    // Lowest value representable in half precision is 2^-24
    // 0 00000 0000000001
    // 2^-14 (denorm exponent) * 2^-10 (mantissa without implicit 1)
    // 2^-14 * 2^-10 =>  2^-24
    //
    const T min_cutoff = -24.0f16;
    const SignedType cond2 = x < min_cutoff;
    result = __abacus_select(result, 0.0f16, cond2);

    return result;
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct helper<T, abacus_float> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    // Cody & Waite reduction
    const SignedType k = abacus::internal::floor_unsafe(x);
    const T r = x - abacus::detail::cast::convert<T>(k);

    const abacus_float polynomial[9] = {1.0f,
                                        6.93147182464599609375e-01f,
                                        2.402265071868896484375E-1f,
                                        5.55040724575519561767578125E-2f,
                                        9.618326090276241302490234375E-3f,
                                        1.33276020642369985580444335938E-3f,
                                        1.55074274516664445400238037109E-4f,
                                        1.42173239510157145559787750244E-5f,
                                        1.85865872026624856516718864441E-6f};

    // Minimax polynomial approximation in the domain [0.0, 1.0]
    // ideal maximum error +8.256512799094739296267877813873319906884e-13
    T result = abacus::internal::horner_polynomial(r, polynomial);

    result = __abacus_ldexp(result, k);

    result = __abacus_select(result, ABACUS_INFINITY, x > 128.0f);

    return __abacus_select(result, 0.0f, x < -149.0f);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct helper<T, abacus_double> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    const T denorm_cutoff = -1077.0;
    // the largest number possible when doing ldexp(<1.0 <= double>, 1024)
    const T max_cutoff = 1024.0;

    const SignedType k = abacus::internal::floor_unsafe(x);
    const T r = x - abacus::detail::cast::convert<T>(k);

    // Minimax polynomial approximation in the domain [0.0, 1.0]
    // ideal maximum error +8.256512799094739296267877813873319906884e-13
    const abacus_double polynomial[13] = {1.0,
                                          0.69314718055994530922926,
                                          0.24022650695910076779888,
                                          0.55504108664818882531371e-1,
                                          0.96181291076796640142576e-2,
                                          0.13333558141426670677961e-2,
                                          0.15403530680872090895962e-3,
                                          0.15252723360978527381725e-4,
                                          0.13215735650922326263223e-5,
                                          1.0174147024447104366395e-7,
                                          7.0958269958400415277169e-9,
                                          4.1791513827899285111307e-10,
                                          3.5365538828220154193844e-11};

    const T fract = abacus::internal::horner_polynomial(r, polynomial);

    const T factor1 = abacus::detail::cast::as<T>((k / 2 + 1023) << 52);
    const T factor2 = abacus::detail::cast::as<T>((k - (k / 2) + 1023) << 52);

    T result = fract * factor1 * factor2;

    const SignedType cond1 = max_cutoff < x;
    result = __abacus_select(result, (T)ABACUS_INFINITY, cond1);

    const SignedType cond2 = denorm_cutoff > x;
    result = __abacus_select(result, (T)0.0, cond2);

    return result;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T exp2(const T x) {
  return helper<T>::_(x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_exp2(abacus_half x) { return exp2<>(x); }
abacus_half2 ABACUS_API __abacus_exp2(abacus_half2 x) { return exp2<>(x); }
abacus_half3 ABACUS_API __abacus_exp2(abacus_half3 x) { return exp2<>(x); }
abacus_half4 ABACUS_API __abacus_exp2(abacus_half4 x) { return exp2<>(x); }
abacus_half8 ABACUS_API __abacus_exp2(abacus_half8 x) { return exp2<>(x); }
abacus_half16 ABACUS_API __abacus_exp2(abacus_half16 x) { return exp2<>(x); }
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_exp2(abacus_float x) { return exp2<>(x); }
abacus_float2 ABACUS_API __abacus_exp2(abacus_float2 x) { return exp2<>(x); }
abacus_float3 ABACUS_API __abacus_exp2(abacus_float3 x) { return exp2<>(x); }
abacus_float4 ABACUS_API __abacus_exp2(abacus_float4 x) { return exp2<>(x); }
abacus_float8 ABACUS_API __abacus_exp2(abacus_float8 x) { return exp2<>(x); }
abacus_float16 ABACUS_API __abacus_exp2(abacus_float16 x) { return exp2<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_exp2(abacus_double x) { return exp2<>(x); }
abacus_double2 ABACUS_API __abacus_exp2(abacus_double2 x) { return exp2<>(x); }
abacus_double3 ABACUS_API __abacus_exp2(abacus_double3 x) { return exp2<>(x); }
abacus_double4 ABACUS_API __abacus_exp2(abacus_double4 x) { return exp2<>(x); }
abacus_double8 ABACUS_API __abacus_exp2(abacus_double8 x) { return exp2<>(x); }
abacus_double16 ABACUS_API __abacus_exp2(abacus_double16 x) {
  return exp2<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
