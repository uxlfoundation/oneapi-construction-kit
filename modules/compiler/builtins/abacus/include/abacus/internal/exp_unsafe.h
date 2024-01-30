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

#ifndef __ABACUS_INTERNAL_EXP_UNSAFE_H__
#define __ABACUS_INTERNAL_EXP_UNSAFE_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>
#include <abacus/internal/floor_unsafe.h>
#include <abacus/internal/horner_polynomial.h>
#include <abacus/internal/ldexp_unsafe.h>

namespace abacus {
namespace internal {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct exp_unsafe_helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct exp_unsafe_helper<T, abacus_half> {
  typedef typename TypeTraits<T>::SignedType SignedType;
  static T _(const T &x) {
    // Find k for Cody & Waite range reduction algorithm
    const T ln2_rcp = 1.44238f16;  // 1.0 / ln(2);
    const T cody = x * ln2_rcp + 0.5f16;
    const SignedType k = abacus::internal::floor_unsafe(cody);

    // Range reducing coerces input into range [0,C], Where C1 + C2 = C.
    // For `exp` our C is `ln(2)`. We've made C1 larger(and C2 smaller)
    // compared to the float implementation so that C1 has more trailing
    // mantissa zero bits, as we want C1 * k to be as accurate as possible.
    const T codyWaite1 = 0.695312f16;     // 0x3990
    const T codyWaite2 = -0.00216484f16;  // 0x986F
    const T kf = abacus::detail::cast::convert<T>(k);

    // Range reduced input value
    const T rr_x = (x - (codyWaite1 * kf)) - (codyWaite2 * kf);

    // See exp.sollya for derivation of polynomial coefficients
    const abacus_half polynomial[6] = {1.0f16,
                                       1.0f16,
                                       0.5f16,
                                       0.1668701171875f16,
                                       4.0374755859375e-2f16,
                                       1.0833740234375e-2f16};

    // Minimax polynomial approximation in the domain [0, ln(2)] of e^x
    const T result = abacus::internal::horner_polynomial(rr_x, polynomial);

    // Polynomial approximation * 2^k
    return abacus::internal::ldexp_unsafe(result, k);
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct exp_unsafe_helper<T, abacus_float> {
  static T _(const T &x) {
    const T codyWaite1 = 0.693359375f;
    const T codyWaite2 = -2.12194440e-4f;
    const T ln2rcp = 1.44269502162933349609375f;

    // 0.5f is just to force k into the range [-0.5f, 0.5f]
    const typename TypeTraits<T>::SignedType k =
        abacus::internal::floor_unsafe(x * ln2rcp + 0.5f);
    const T kf = abacus::detail::cast::convert<T>(k);
    const T r = (x - (codyWaite1 * kf)) - (codyWaite2 * kf);

    const abacus_float polynomial[10] = {
        1.00000000001102f,     0.999999999895083f,    0.499999998748109f,
        0.166666669553853f,    0.416666887017172e-1f, 0.833331253849749e-2f,
        0.138875551530099e-2f, 0.198464467953104e-3f, 0.251273731825286e-4f,
        0.272579824216659e-5f};

    // minimax from 0 -> ln(2) of e^x
    const T result = abacus::internal::horner_polynomial(r, polynomial);

    return abacus::internal::ldexp_unsafe(result, k);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct exp_unsafe_helper<T, abacus_double> {
  static T _(const T &x) {
    const T codyWaite1 = abacus::detail::cast::as<abacus_double>(
        (abacus_long)0x3FE62E42FEFA3800);
    const T codyWaite2 = abacus::detail::cast::as<abacus_double>(
        (abacus_long)0x3D2EF35793C76000);
    const T codyWaite3 =
        1.16122272293625318506558032769199466281698772531037279187308633269964186875e-26;
    const T ln2rcp =
        1.44269504088896340735992468100189213742664595415298593413544;

    const typename TypeTraits<T>::SignedType k =
        abacus::internal::floor_unsafe(x * ln2rcp);
    const T kf = abacus::detail::cast::convert<T>(k);
    const T r = ((x - kf * codyWaite1) - kf * codyWaite2) - kf * codyWaite3;

    const abacus_double polynomial[15] = {0.100000000000000000004072260342e1,
                                          0.999999999999999979186940414675e0,
                                          0.500000000000001749954086847939e0,
                                          0.166666666666609202887577905096e0,
                                          0.416666666676419918920300528242e-1,
                                          0.833333332352719704910828298982e-2,
                                          0.138888895189922619737855230932e-2,
                                          0.198412428281155364932818758727e-3,
                                          0.248023762714154066356240502304e-4,
                                          0.275415872847843118152695604532e-5,
                                          0.277674669926111249476681443055e-6,
                                          0.232649531208374020799752875179e-7,
                                          0.294609311301038779771435680411e-8};

    // minimax from 0 -> ln(2) of e^x
    const T result = abacus::internal::horner_polynomial(r, polynomial);

    return abacus::internal::ldexp_unsafe(result, k);
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
inline T exp_unsafe(const T &x) {
  return exp_unsafe_helper<T>::_(x);
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_EXP_UNSAFE_H__
