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
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct helper<T, abacus_half> {
  typedef typename TypeTraits<T>::SignedType SignedType;
  static T _(const T x) {
    // See expm1.sollya for derivation of polynomial coefficients
    // Replaced -1.26965460367500782012939453125e-9 with -0.0 since it can't be
    // represented in half precision.
    const abacus_half polynomial[10] = {-0.0f16,
                                        1.0f16,
                                        0.5f16,
                                        0.1666259765625f16,
                                        4.1656494140625e-2f16,
                                        8.544921875e-3f16,
                                        1.3103485107421875e-3f16,
                                        -7.8022480010986328125e-5f16,
                                        2.863407135009765625e-4f16,
                                        -6.186962127685546875e-5f16};

    // Use polynomial within bound [-0.6, 1.7] where we need to be more precise
    T result = abacus::internal::horner_polynomial(x, polynomial);

    // Use naive `exp() - 1` implementation outwith bounds [-0.6, 1.7]
    const T naive = __abacus_exp(x) - 1.0f16;
    const SignedType bound = (x < -0.6f16) | (x > 1.7f16);
    result = __abacus_select(result, naive, bound);

    return result;
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct helper<T, abacus_float> {
  static T _(const T x) {
    const abacus_float polynomial[10] = {0.0f,
                                         0.9999999977f,
                                         0.5000000215f,
                                         0.1666667604f,
                                         0.4166633777e-1f,
                                         0.8332837778e-2f,
                                         0.1390427397e-2f,
                                         0.1985272421e-3f,
                                         0.2259264801e-4f,
                                         0.4331310997e-5f};

    const T result = abacus::internal::horner_polynomial(x, polynomial);

    return __abacus_select(result, __abacus_exp(x) - 1.0f,
                           (x < -0.6f) | (x > 1.60809791088104248046875f));
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct helper<T, abacus_double> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    // see maple worksheet for polynomial derivation using (exp(x) - 1) / x
    const abacus_double polynomial1[15] = {
        0.99999999999999999999992965e0,  0.49999999999999999996919203e0,
        0.16666666666666666441824501e0,  0.41666666666666601682792959e-1,
        0.83333333333323469577984284e-2, 0.13888888888798502555164065e-2,
        0.19841269835851916759158229e-3, 0.24801587078430463537070438e-4,
        0.27557312715420426754154154e-5, 0.27557182596311394844419535e-6,
        0.25050034322322299212970925e-7, 0.20854147209633266285188932e-8,
        0.15885803778359846816278330e-9, 0.10577096483033681328099693e-10,
        0.47890864373018777092185793e-12};

    const abacus_double polynomial2[14] = {
        0.99999999999999999999970449e0,  0.50000000000000000014847500e0,
        0.16666666666666665434502956e0,  0.41666666666667068132840177e-1,
        0.83333333333265271323600137e-2, 0.13888888889578243572712637e-2,
        0.19841269796154398869625868e-3, 0.24801589300380590631496421e-4,
        0.27557257692870410304420023e-5, 0.27558648899712529981103364e-6,
        0.25032039629301642256493113e-7, 0.21083281545050501806939634e-8,
        0.14687476194541863805776504e-9, 0.16664262059965519086145341e-10};

    const abacus_double polynomial3[15] = {
        0.3735204080085689378313e-10,    0.99999999951800677539362361e0,
        0.50000000288575061048638471e0,  0.16666665603460921713543987e0,
        0.4166669361951862603243932e-1,  0.83332835472710443633852766e-2,
        0.13889580865632589359789001e-2, 0.19833904443359213015936229e-3,
        0.24862070960732964397733931e-4, 0.27174603500942085037745977e-5,
        0.29404238712769949070933858e-6, 0.18401646920614041116105154e-7,
        0.38042916302360776511096830e-8, -0.13389158390225752109605929e-9,
        0.39795148261111660511008232e-10};

    const SignedType cond1 = (x > -1.0) & (x <= 0.0);

    T result = __abacus_select(
        __abacus_exp(x) - 1.0,
        x * abacus::internal::horner_polynomial(x, polynomial1), cond1);

    const SignedType cond2 = (x > 0.0) & (x <= 0.8);

    result = __abacus_select(
        result, x * abacus::internal::horner_polynomial(x, polynomial2), cond2);

    const SignedType cond3 = (x > 0.8) & (x < 1.7);

    result = __abacus_select(
        result, abacus::internal::horner_polynomial(x, polynomial3), cond3);

    return result;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T expm1(const T x) {
  return helper<T>::_(x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_expm1(abacus_half x) { return expm1<>(x); }
abacus_half2 ABACUS_API __abacus_expm1(abacus_half2 x) { return expm1<>(x); }
abacus_half3 ABACUS_API __abacus_expm1(abacus_half3 x) { return expm1<>(x); }
abacus_half4 ABACUS_API __abacus_expm1(abacus_half4 x) { return expm1<>(x); }
abacus_half8 ABACUS_API __abacus_expm1(abacus_half8 x) { return expm1<>(x); }
abacus_half16 ABACUS_API __abacus_expm1(abacus_half16 x) { return expm1<>(x); }
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_expm1(abacus_float x) { return expm1<>(x); }
abacus_float2 ABACUS_API __abacus_expm1(abacus_float2 x) { return expm1<>(x); }
abacus_float3 ABACUS_API __abacus_expm1(abacus_float3 x) { return expm1<>(x); }
abacus_float4 ABACUS_API __abacus_expm1(abacus_float4 x) { return expm1<>(x); }
abacus_float8 ABACUS_API __abacus_expm1(abacus_float8 x) { return expm1<>(x); }
abacus_float16 ABACUS_API __abacus_expm1(abacus_float16 x) {
  return expm1<>(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_expm1(abacus_double x) { return expm1<>(x); }
abacus_double2 ABACUS_API __abacus_expm1(abacus_double2 x) {
  return expm1<>(x);
}
abacus_double3 ABACUS_API __abacus_expm1(abacus_double3 x) {
  return expm1<>(x);
}
abacus_double4 ABACUS_API __abacus_expm1(abacus_double4 x) {
  return expm1<>(x);
}
abacus_double8 ABACUS_API __abacus_expm1(abacus_double8 x) {
  return expm1<>(x);
}
abacus_double16 ABACUS_API __abacus_expm1(abacus_double16 x) {
  return expm1<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
