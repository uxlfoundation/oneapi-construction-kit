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

#ifndef __ABACUS_INTERNAL_SINCOS_APPROX_H__
#define __ABACUS_INTERNAL_SINCOS_APPROX_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/horner_polynomial.h>

// see sincos sollya script for how coefficients were derived.
#ifdef __CA_BUILTINS_HALF_SUPPORT
static ABACUS_CONSTANT abacus_half _sincos_coefcH[] = {
    1.0f16, -0.308349609375f16, 1.56402587890625e-2f16,
    -1.8084049224853515625e-4f16};
static ABACUS_CONSTANT abacus_half _sincos_coefsH[] = {
    0.78515625f16, -7.8857421875e-2f16, -1.2340545654296875e-3f16,
    2.0732879638671875e-3f16};
#endif  // __CA_BUILTINS_HALF_SUPPORT

static ABACUS_CONSTANT abacus_float _sincos_coefc[] = {
    1.0f, -0.5f, 4.16666455566883087158203125e-2f,
    -1.388731063343584537506103515625e-3f,
    2.443256744300015270709991455078125e-5f};
static ABACUS_CONSTANT abacus_float _sincos_coefs[] = {
    1.0f, -0.1666665375232696533203125f, 8.332121185958385467529296875e-3f,
    -1.951101585291326045989990234375e-4f};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
static ABACUS_CONSTANT abacus_double _sincos_coefcD[] = {
    1.0,
    -0.30842513753404243725952937893453054130077362060547,
    1.5854344243815179232859335911598464008420705795288e-2,
    -3.259918869254246269299979399391986589762382209301e-4,
    3.590860442577276323100577901104024647338519571349e-6,
    -2.46113593161431293205938157595685789047479374858085e-8,
    1.15001885209734771641010282085254698078435886543502e-10,
    -3.8493926173412299468153000715267938102934208721706e-13};
static ABACUS_CONSTANT abacus_double _sincos_coefsD[] = {
    0.78539816339744827899949086713604629039764404296875,
    -8.0745512188279758292175358747044811025261878967285e-2,
    2.4903945701826297846881441699906645226292312145233e-3,
    -3.6576204137696543801719362143387570540653541684151e-5,
    3.1336158729412052553474116352305589572324606706388e-7,
    -1.75712194750903267584560913451867045220744500966248e-9,
    6.868724252837273796682665631745825607183675298728e-12};
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
