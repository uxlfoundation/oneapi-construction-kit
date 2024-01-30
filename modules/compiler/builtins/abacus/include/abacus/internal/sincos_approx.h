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

#ifndef __ABACUS_INTERNAL_SINCOS_APPROX_H__
#define __ABACUS_INTERNAL_SINCOS_APPROX_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/horner_polynomial.h>

// see maple worksheets for how coefficients were derived.
#ifdef __CA_BUILTINS_HALF_SUPPORT
// see sincos sollya script for half derivations
static ABACUS_CONSTANT abacus_half _sincos_coefcH[4] = {
    1.0f16, -0.308349609375f16, 1.56402587890625e-2f16,
    -1.8084049224853515625e-4f16};
static ABACUS_CONSTANT abacus_half _sincos_coefsH[4] = {
    0.78515625f16, -7.8857421875e-2f16, -1.2340545654296875e-3f16,
    2.0732879638671875e-3f16};
#endif  // __CA_BUILTINS_HALF_SUPPORT

static ABACUS_CONSTANT abacus_float _sincos_coefc[4] = {
    1.0f, -0.4999988475f, 0.4165577706e-1f, -0.1359185355e-2f};
static ABACUS_CONSTANT abacus_float _sincos_coefs[4] = {
    0.9999999969f, -0.1666665022f, 0.008332016456f, -0.0001950182203f};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
static ABACUS_CONSTANT abacus_double _sincos_coefcD[7] = {
    0.9999999999999999442900209e0,  -0.3084251375340372298786989e0,
    0.1585434424373456820405343e-1, -0.3259918864540400178041973e-3,
    0.3590859123360384879050480e-5, -0.2460945716617648723210627e-7,
    0.1136381269877847714456938e-9};
static ABACUS_CONSTANT abacus_double _sincos_coefsD[7] = {
    0.7853981633974483070143883e0,  -0.8074551218828053020239718e-1,
    0.2490394570188736259128287e-2, -0.3657620415845638796283708e-4,
    0.3133616216633753227573597e-6, -0.1757149294158822577565553e-8,
    0.6877100843155614304321773e-11};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

namespace abacus {
namespace internal {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct sincos_approx_helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct sincos_approx_helper<T, abacus_half> {
  static T _(const T &x, T *out_cos) {
    using UnsignedType = typename TypeTraits<T>::UnsignedType;

    const T xx = x * x;
    *out_cos = abacus::internal::horner_polynomial(xx, _sincos_coefcH);

    const T sin = x * abacus::internal::horner_polynomial(xx, _sincos_coefsH);

    // 0.151611328125 (0x30da) is only slightly above 2 ULP. It's faster to
    // add a special case for this input using select instead of changing the
    // algorithm significantly.
    return __abacus_select(sin,
                           abacus::detail::cast::as<T>(UnsignedType(0x2f9b)),
                           UnsignedType(x == 0.151611328125f16));
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct sincos_approx_helper<T, abacus_float> {
  static T _(const T &x, T *out_cos) {
    const T xx = x * x;
    *out_cos = abacus::internal::horner_polynomial(xx, _sincos_coefc);

    return x * abacus::internal::horner_polynomial(xx, _sincos_coefs);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct sincos_approx_helper<T, abacus_double> {
  static T _(const T &x, T *out_cos) {
    const T xx = x * x;
    *out_cos = abacus::internal::horner_polynomial(xx, _sincos_coefcD);

    return x * abacus::internal::horner_polynomial(xx, _sincos_coefsD);
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
inline T sincos_approx(T x, T *out_cos) {
  return sincos_approx_helper<T>::_(x, out_cos);
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_SINCOS_APPROX_H__
