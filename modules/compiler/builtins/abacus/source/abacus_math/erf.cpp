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
#include <abacus/internal/horner_polynomial.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct erf_helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct erf_helper<T, abacus_half> {
  using SignedType = typename TypeTraits<T>::SignedType;

  static T _(const T x) {
    const T xAbs = __abacus_fabs(x);

    // Polynomial approximations of 'erf(abs(x)) / abs(x)' across input
    // thresholds. See erf.sollya for the derivations.

    // Polynomial over range [0, 0.8]
    const abacus_half polynomial0[4] = {1.1279296875f16, 1.10321044921875e-2f16,
                                        -0.44189453125f16, 0.1436767578125f16};
    const T s0 = abacus::internal::horner_polynomial(xAbs, polynomial0);

    // Polynomial over range [0.8, 1.75]
    const abacus_half polynomial1[4] = {1.234375f16, -0.2978515625f16,
                                        -0.15478515625f16,
                                        6.0638427734375e-2f16};
    const T s1 = abacus::internal::horner_polynomial(xAbs, polynomial1);

    // Polynomial over range [1.75, 2.1]
    const abacus_half polynomial2[3] = {1.40234375f16, -0.66650390625f16,
                                        0.1070556640625f16};
    const T s2 = abacus::internal::horner_polynomial(xAbs, polynomial2);

    // Select the last interval as the default value
    T result = s2;
    result = __abacus_select(result, s1, SignedType(xAbs < 1.75f16));
    result = __abacus_select(result, s0, SignedType(xAbs < 0.8f16));

    result *= xAbs;
    result = __abacus_copysign(result, x);

    // erf() has 4 ULP of allowed error in cl_khr_fp16 spec. As |erf()|
    // converges to 1.0 (0x3C00) as |x| increases, we can round the result
    // to 1.0 if the result of the reference function is >= 0.998046875
    // (0x3BFC), which is within 4 ULP of 1.0.
    //
    // Solving erf(x) = 0.998046875, we get x ~= 2.19009996835376763823687941...
    // or 2.189453125 (0x4061) RTE rounded. We therefore choose this as our
    // threshold to return exactly 1.0 or -1.0, depending on the sign of x.
    result = __abacus_select(result, __abacus_copysign(1.0f16, x),
                             SignedType(xAbs > 2.189453125f16));

    result = __abacus_select(result, x, SignedType(__abacus_isnan(x)));

    return result;
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct erf_helper<T, abacus_float> {
  static T _(const T x) {
    const T xAbs = __abacus_fabs(x);

    // xAbs < 0.8,  interval = 0
    // xAbs < 1.75, interval = 1
    // xAbs < 2.8,  interval = 2
    // otherwise,   interval = 3

    // see maple worksheet for polynomial derivation
    const abacus_float polynomial0[7] = {
        1.128379453136f,     -0.340827090e-4f, -.3754641048231f,
        -0.480089943431e-2f, .1291708894549f,  -0.2738900335275e-1f,
        -0.7265839921372e-2f};

    const T s0 = xAbs * abacus::internal::horner_polynomial(xAbs, polynomial0);

    const abacus_float polynomial1[7] = {
        .1572988043812f,     -.4151063740764f,     .4151681759610f,
        -.1386338728462f,    -0.6996173760407e-1f, 0.7588668092134e-1f,
        -0.1999414841696e-1f};

    const T s1 =
        (T)1.0f - abacus::internal::horner_polynomial(xAbs - 1.0f, polynomial1);

    const abacus_float polynomial2[7] = {
        0.4677923940847e-2f,  -0.2066698149162e-1f, 0.4131261993220e-1f,
        -0.4814201425671e-1f, 0.3459644561569e-1f,  -0.1449449409692e-1f,
        0.2739553512239e-2f};

    const T s2 =
        (T)1.0f - abacus::internal::horner_polynomial(xAbs - 2.0f, polynomial2);

    const abacus_float polynomial3[7] = {
        0.221830562446e-4f,  -0.140925585762e-3f, 0.420115643592e-3f,
        -0.743785588448e-3f, 0.804037923904e-3f,  -0.488747786554e-3f,
        0.127400179584e-3f};

    const T s3 =
        (T)1.0f - abacus::internal::horner_polynomial(xAbs - 3.0f, polynomial3);

    // select the last interval as the default value
    T result = s3;
    result = __abacus_select(result, s2, xAbs < 2.8f);
    result = __abacus_select(result, s1, xAbs < 1.75f);
    result = __abacus_select(result, s0, xAbs < 0.8f);

    result = __abacus_copysign(result, x);

    result = __abacus_select(result, __abacus_copysign(1.0f, x),
                             xAbs > 3.8325068950653076171875f);

    result = __abacus_select(result, x, __abacus_isnan(x));

    return result;
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct erf_helper<T, abacus_double> {
  using SignedType = typename TypeTraits<T>::SignedType;

  static T _(const T x) {
    const T xAbs = __abacus_fabs(x);

    const T erfVal = (T)1.0 - __abacus_erfc(xAbs);

    T result = __abacus_copysign(erfVal, x);

    // see maple worksheet for polynomial derivation
    const abacus_double polynomial[14] = {
        0.1128379167095512573899991e1,   -0.50328494e-17,
        -0.3761263890318364281377628e0,  -0.941146567158939e-13,
        0.1128379167137682207047200e0,   -0.1132505028214632772620e-9,
        -0.2686616867304819308121787e-1, -0.23338189269890228764e-7,
        0.5224170445789120756274344e-2,  -0.1125279925284428513367e-5,
        -0.8502053102671020146012240e-3, -0.131562968994956982451090e-4,
        0.1453015580982363557541757e-3,  -0.2803416013593745284248751e-4};

    const T s = x * abacus::internal::horner_polynomial(xAbs, polynomial);

    const SignedType c0 = xAbs < 0.3;
    result = __abacus_select(result, s, c0);

    const SignedType c1 = xAbs > 5.863584748755167927207662585832784164;
    result = __abacus_select(result, __abacus_copysign(1.0, x), c1);

    return result;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
inline T erf(const T x) {
  return erf_helper<T>::_(x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_erf(abacus_half x) { return erf<>(x); }
abacus_half2 ABACUS_API __abacus_erf(abacus_half2 x) { return erf<>(x); }
abacus_half3 ABACUS_API __abacus_erf(abacus_half3 x) { return erf<>(x); }
abacus_half4 ABACUS_API __abacus_erf(abacus_half4 x) { return erf<>(x); }
abacus_half8 ABACUS_API __abacus_erf(abacus_half8 x) { return erf<>(x); }
abacus_half16 ABACUS_API __abacus_erf(abacus_half16 x) { return erf<>(x); }
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_erf(abacus_float x) { return erf<>(x); }
abacus_float2 ABACUS_API __abacus_erf(abacus_float2 x) { return erf<>(x); }
abacus_float3 ABACUS_API __abacus_erf(abacus_float3 x) { return erf<>(x); }
abacus_float4 ABACUS_API __abacus_erf(abacus_float4 x) { return erf<>(x); }
abacus_float8 ABACUS_API __abacus_erf(abacus_float8 x) { return erf<>(x); }
abacus_float16 ABACUS_API __abacus_erf(abacus_float16 x) { return erf<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_erf(abacus_double x) { return erf<>(x); }
abacus_double2 ABACUS_API __abacus_erf(abacus_double2 x) { return erf<>(x); }
abacus_double3 ABACUS_API __abacus_erf(abacus_double3 x) { return erf<>(x); }
abacus_double4 ABACUS_API __abacus_erf(abacus_double4 x) { return erf<>(x); }
abacus_double8 ABACUS_API __abacus_erf(abacus_double8 x) { return erf<>(x); }
abacus_double16 ABACUS_API __abacus_erf(abacus_double16 x) { return erf<>(x); }
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
