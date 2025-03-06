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

#ifndef __ABACUS_INTERNAL_LGAMMA_POSITIVE_H__
#define __ABACUS_INTERNAL_LGAMMA_POSITIVE_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>
#include <abacus/internal/horner_polynomial.h>

namespace abacus {
namespace internal {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct lgamma_traits;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct lgamma_traits<T, abacus_half> {
  static constexpr abacus_half one_over_pi = ABACUS_1_PI_H;
  static const abacus_ushort overflow_limit = 0x6ffd;   // 8180.0
  static const abacus_ushort underflow_limit = 0xef30;  // -7360.0
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct lgamma_traits<T, abacus_float> {
  static constexpr abacus_float one_over_pi = ABACUS_1_PI_F;
  static const abacus_uint overflow_limit = 0x7c44af8d;
  static const abacus_uint underflow_limit = 0xcb000000;
};

namespace {
#ifdef __CA_BUILTINS_HALF_SUPPORT
static ABACUS_CONSTANT abacus_half _lgamma_translation_half[8] = {
    0.0f16, 1.0f16, 1.0f16, 2.0f16, 2.0f16, 3.0f16, 5.0f16, 8.0f16};

static ABACUS_CONSTANT abacus_half _intervals_half[8] = {
    0.0f16, 0.8f16, 1.0f16, 1.5f16, 2.0f16, 3.0f16, 5.0f16, 8.0f16};

static ABACUS_CONSTANT abacus_half __codeplay_lgamma_positive_coeff_half[64] = {
    // Interval 1
    0.0f16, -.5772093071f16, .8222480525f16, -.3980991068f16, .2556843726f16,
    -.1589499899f16, 0.7297015401e-1f16, -0.1669297533e-1f16,
    // Interval 2
    0.0f16, -.5772156590e0f16, 0.8224697880e0f16, -0.4005254160e0f16,
    0.2739341320e0f16, -0.1764588220e0f16, 0.2935728070e0f16, 0.0f16,
    // Interval 3
    0.0f16, -0.5772156291e0f16, 0.8224598249e0f16, -0.4004593440e0f16,
    0.2678333591e0f16, -0.1908303994e0f16, 0.1145646090e0f16,
    -0.3816682859e-1f16,
    // Interval 4
    0.0f16, 0.4227843334e0f16, 0.3224667330e0f16, -0.6736112525e-1f16,
    0.2048328921e-1f16, -0.7895180249e-2f16, 0.1557123540e-2f16,
    -0.2796116231e-2f16,
    // Interval 5
    0.0f16, 0.4227845308e0f16, 0.3224612870e0f16, -0.6729584497e-1f16,
    0.2031713177e-1f16, -0.6712504896e-2f16, 0.1889833929e-2f16,
    -0.2972583504e-3f16,
    // Interval 6
    0.6931471708f16, .9227851390f16, .1974564876f16, -0.2563424834e-1f16,
    0.4833391309e-2f16, -0.9735489234e-3f16, 0.1588195326e-3f16,
    -0.1375602406e-4f16,
    // Interval 7
    0.3178053822e1f16, 0.1506118088e1f16, 0.1106580532e0f16,
    -0.8121088385e-2f16, 0.8768583322e-3f16, -0.1037094728e-3f16,
    0.1028315045e-4f16, -0.5580142414e-6f16,
    // Interval 8
    0.8525159415e1f16, 0.2015670265e1f16, 0.6650204212e-1f16,
    -0.2892357977e-2f16, 0.1708553190e-3f16, -0.9290151673e-5f16,
    0.3465180358e-6f16, -5.9605E-8f16};
#endif  // __CA_BUILTINS_HALF_SUPPORT

static ABACUS_CONSTANT abacus_float _lgamma_translation[8] = {
    0.0f, 1.0f, 1.0f, 2.0f, 2.0f, 3.0f, 5.0f, 8.0f};

static ABACUS_CONSTANT abacus_float _intervals[8] = {0.0f, 0.8f, 1.0f, 1.5f,
                                                     2.0f, 3.0f, 5.0f, 8.0f};

static ABACUS_CONSTANT abacus_float __codeplay_lgamma_positive_coeff[64] = {
    // Interval 1
    0.0f, -.5772093071f, .8222480525f, -.3980991068f, .2556843726f,
    -.1589499899f, 0.7297015401e-1f, -0.1669297533e-1f,
    // Interval 2
    0.0f, -.5772156590e0f, 0.8224697880e0f, -0.4005254160e0f, 0.2739341320e0f,
    -0.1764588220e0f, 0.2935728070e0f, 0.0f,
    // Interval 3
    0.0f, -0.5772156291e0f, 0.8224598249e0f, -0.4004593440e0f, 0.2678333591e0f,
    -0.1908303994e0f, 0.1145646090e0f, -0.3816682859e-1f,
    // Interval 4
    0.0f, 0.4227843334e0f, 0.3224667330e0f, -0.6736112525e-1f, 0.2048328921e-1f,
    -0.7895180249e-2f, 0.1557123540e-2f, -0.2796116231e-2f,
    // Interval 5
    0.0f, 0.4227845308e0f, 0.3224612870e0f, -0.6729584497e-1f, 0.2031713177e-1f,
    -0.6712504896e-2f, 0.1889833929e-2f, -0.2972583504e-3f,
    // Interval 6
    0.6931471708f, .9227851390f, .1974564876f, -0.2563424834e-1f,
    0.4833391309e-2f, -0.9735489234e-3f, 0.1588195326e-3f, -0.1375602406e-4f,
    // Interval 7
    0.3178053822e1f, 0.1506118088e1f, 0.1106580532e0f, -0.8121088385e-2f,
    0.8768583322e-3f, -0.1037094728e-3f, 0.1028315045e-4f, -0.5580142414e-6f,
    // Interval 8
    0.8525159415e1f, 0.2015670265e1f, 0.6650204212e-1f, -0.2892357977e-2f,
    0.1708553190e-3f, -0.9290151673e-5f, 0.3465180358e-6f, -0.6069832791e-8f};
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
inline abacus_half lgamma_positive(abacus_half x) {
  abacus_half logx = __abacus_log(x);

  if (x > 20.0f16) {
    return ((((x - 0.5f16) * logx) - x) + (0.083313f16 / x + 0.918945f16));
  }
  abacus_short high_four = 4 * (x > _intervals_half[4]);
  abacus_short high_two = high_four + 2 * (x > _intervals_half[high_four + 2]);
  abacus_short interval = high_two + (x > _intervals_half[high_two + 1]);

  x -= _lgamma_translation_half[interval];

  ABACUS_CONSTANT abacus_half *coef_ptr =
      __codeplay_lgamma_positive_coeff_half + interval * (size_t)8;
  abacus_half semi = abacus::internal::horner_polynomial(x, coef_ptr, 8);

  return (interval) ? semi : semi - logx;
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

inline abacus_float lgamma_positive(abacus_float x) {
  const abacus_float logx = __abacus_log(x);

  if (x > 20.0f) {
    if (x >= abacus::detail::cast::as<abacus_float>(abacus_uint(0x7c42613a))) {
      return x * (logx - 1.0f);
    }

    return ((((x - 0.5f) * logx) - x) +
            (0.08333333333f / x + 0.91893851757049560546875f));
  }
  const abacus_int high_four = 4 * (x > _intervals[4]);
  const abacus_int high_two = high_four + (2 * (x > _intervals[high_four + 2]));
  const abacus_int interval = high_two + (x > _intervals[high_two + 1]);

  x -= _lgamma_translation[interval];

  const abacus_float semi = abacus::internal::horner_polynomial(
      x, __codeplay_lgamma_positive_coeff + (interval * (size_t)8), 8);

  return (interval) ? semi : semi - logx;
}

template <typename T>
inline T lgamma_positive(const T &x) {
  using SignedType = typename TypeTraits<T>::SignedType;
  using UnsignedType = typename TypeTraits<T>::UnsignedType;
  using UnsignedElementType = typename TypeTraits<UnsignedType>::ElementType;

  const T logx = __abacus_log(x);

  UnsignedType interval = 0;
  T ans = 0;

  for (UnsignedElementType i = 0; i < 8; i++) {
    const SignedType cond = x > _intervals[i];
    interval = __abacus_select(interval, i, cond);

    const T poly = abacus::internal::horner_polynomial(
        x - _lgamma_translation[i],
        __codeplay_lgamma_positive_coeff + (i * (size_t)8), 8);

    ans = __abacus_select(ans, poly, cond);
  }

  T result = __abacus_select(ans - logx, ans, interval != 0);

  result =
      __abacus_select(result,
                      ((((x - 0.5f) * logx) - x) +
                       ((T)0.08333333333f / x + 0.91893851757049560546875f)),
                      x > 20.0f);

  result = __abacus_select(
      result, x * (logx - 1.0f),
      x >= abacus::detail::cast::as<T>(UnsignedType(0x7c42613a)));

  return result;
}

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
inline T lgamma_positive_half(const T &x) {
  using SignedType = typename TypeTraits<T>::SignedType;
  using UnsignedType = typename TypeTraits<T>::UnsignedType;
  using UnsignedElementType = typename TypeTraits<UnsignedType>::ElementType;

  const T logx = __abacus_log(x);

  UnsignedType interval = 0;
  T ans = 0;

  for (UnsignedElementType i = 0; i < 8; i++) {
    const SignedType cond = x > _intervals_half[i];
    interval = __abacus_select(interval, i, cond);

    const T poly = abacus::internal::horner_polynomial(
        x - _lgamma_translation_half[i],
        __codeplay_lgamma_positive_coeff_half + i * (size_t)8, 8);

    ans = __abacus_select(ans, poly, cond);
  }

  T result = __abacus_select(ans - logx, ans, interval != 0);

  result = __abacus_select(
      result,
      ((((x - 0.5f16) * logx) - x) + ((T)0.083313f16 / x + 0.918945f16)),
      x > 20.0f);

  return result;
}

template <>
inline abacus_half2 lgamma_positive(const abacus_half2 &x) {
  return lgamma_positive_half(x);
}

template <>
inline abacus_half3 lgamma_positive(const abacus_half3 &x) {
  return lgamma_positive_half(x);
}

template <>
inline abacus_half4 lgamma_positive(const abacus_half4 &x) {
  return lgamma_positive_half(x);
}
template <>
inline abacus_half8 lgamma_positive(const abacus_half8 &x) {
  return lgamma_positive_half(x);
}
template <>
inline abacus_half16 lgamma_positive(const abacus_half16 &x) {
  return lgamma_positive_half(x);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
// Gets the value of lgamma in the range [1,2]
inline abacus_double lgamma_1to2(abacus_double x) {
  const abacus_double orig = x;
  abacus_double ans = ABACUS_NAN;
  if (1.0 <= orig && orig <= 1.5) {
    // About 1 ulp compared to std:
    x = x - 1.0;
    // lgamma is basically a random number generator, with no reference
    // function in the CTS. So we don't need to worry about error accumulation
    // in this large expression.
    const abacus_double poly =
        -0.577215664901532860606399608944e0 +
        ((0.822467033424113218033719309828e0 +
          (-0.400685634386531367842803708273e0 +
           (0.270580808427777333564177283854e0 +
            (-0.207385551028218933790777990141e0 +
             (0.169557176979815554158924679926e0 +
              (-0.144049896314135072037356804522e0 +
               (0.125509661206761405444719466196e0 +
                (-0.111334153842271144222894568402e0 +
                 (0.100098314838956727346418246624e0 +
                  (-0.909450071440206661576973282302e-1 +
                   (0.832980965196954546433547142618e-1 +
                    (-0.766590205621694453623593115599e-1 +
                     (0.703610686487618231366489543360e-1 +
                      (-0.632970018535340529257608896951e-1 +
                       (0.539613915512959858827812080122e-1 +
                        (-0.413575207389668799045096002923e-1 +
                         (0.265466543807868911939220884698e-1 +
                          (-0.130043694118914474315358017838e-1 +
                           (0.422529901859797520291282888992e-2 -
                            0.672121132329985303236139757800e-3 * x) *
                               x) *
                              x) *
                             x) *
                            x) *
                           x) *
                          x) *
                         x) *
                        x) *
                       x) *
                      x) *
                     x) *
                    x) *
                   x) *
                  x) *
                 x) *
                x) *
               x) *
              x) *
         x);
    ans = poly * x;
  }

  if (1.5 < orig && orig <= 2.0) {
    // About 1 ulp compared to std:
    x = 2.0 - x;
    const abacus_double poly =
        -0.422784335098467139393487866219e0 +
        ((0.322467033424113218236131449144e0 +
          (0.673523010531980951553028283174e-1 +
           (0.205808084277845453409081136005e-1 +
            (0.738555102867413988255202865380e-2 +
             (0.289051033073576068020666031208e-2 +
              (0.119275391184648988863776614656e-2 +
               (0.509669522231860084260680834947e-3 +
                (0.223154790728439308838879178915e-3 +
                 (0.994572003976975949284111902962e-4 +
                  (0.44928556849107108509758914903e-4 +
                   (0.204938234555901374791249695625e-4 +
                    (0.94999652296367022403396210220e-5 +
                     (0.41605582240358196374085008100e-5 +
                      (0.26334418813466617407682748547e-5 +
                       (-0.3230790013731150352208783542e-6 +
                        (0.254758976379121405515527436892e-5 +
                         (-0.234350817628823776555608301853e-5 +
                          (0.229896826274862502096007889899e-5 +
                           (-0.116894305652024296372739243535e-5 +
                            0.367605658911627977263759472550e-6 * x) *
                               x) *
                              x) *
                             x) *
                            x) *
                           x) *
                          x) *
                         x) *
                        x) *
                       x) *
                      x) *
                     x) *
                    x) *
                   x) *
                  x) *
                 x) *
                x) *
               x) *
              x) *
         x);
    ans = poly * x;
  }

  return ans;
}

inline abacus_double lgamma_positive(abacus_double x) {
  if (x >= 1.0e15) {
    return x * (__abacus_log(x) - 1.0);
  }

  if (x >= 10.0) {
    // Stirlings approximation:
    const abacus_double ln2pi =
        1.837877066409345483560659472811235279722794947275566825634303 * 0.5;
    const abacus_double xx = x * x;
    return ((x - 0.5) * __abacus_log(x)) - x + ln2pi +
           (((((6930.0 * xx - 231.0) * xx + 66.0) * xx - 49.5) * xx + 70.0) /
            (83160.0 * xx * xx * xx * xx * x));
  }

  // G(x) = G(x+1) - ln(x)
  if (x < 1.0) {
    if (x > 0.9) {
      x = 1.0 - x;
      return x * (0.577215664901532860606512084204e0 +
                  (0.822467033424113218236237744050e0 +
                   (0.400685634386531428440905650612e0 +
                    (0.270580808427784556508720968938e0 +
                     (0.207385551028672463804431385438e0 +
                      (0.169557176997570176923582107613e0 +
                       (0.144049896757542853782059082901e0 +
                        (0.125509670068716504156605412200e0 +
                         (0.111334247225507425415510607782e0 +
                          (0.100099921237935757871704915541e0 +
                           (0.909455799116999625373112941462e-1 +
                            (0.834658494141644907067471003415e-1 +
                             (0.758636849209001489849294167345e-1 +
                              (0.785250463125571263115676130825e-1 +
                               (0.36050812051622394145697638300e-1 +
                                0.136577465553874987349856273482e0 * x) *
                                   x) *
                                  x) *
                                 x) *
                                x) *
                               x) *
                              x) *
                             x) *
                            x) *
                           x) *
                          x) *
                         x) *
                        x) *
                       x) *
                      x);
    }
    return lgamma_1to2(x + 1.0) - __abacus_log(x);
  }

  if (1.0 <= x && x <= 2.0) {
    return lgamma_1to2(x);
  }

  if (2.0 < x && x <= 3.0) {
    return lgamma_1to2(x - 1.0) + __abacus_log(x - 1.0);
  }

  if (3.0 < x && x <= 4.0) {
    return lgamma_1to2(x - 2.0) + __abacus_log((x - 1.0) * (x - 2.0));
  }

  if (4.0 < x && x < 10.0) {
    x -= 5.0;
    return 0.317805383034794379153748384411e1 +
           ((0.150611766843180065206245208226e1 +
             (0.110661477868644201072990211035e0 +
              (-0.813162204086589076485758012768e-2 +
               (0.892826174031926137374888255900e-3 +
                (-0.117193260841602955724637693181e-3 +
                 (0.170298763235858759482598081220e-4 +
                  (-0.264212384092237999430596483986e-5 +
                   (0.428950533963169013392955115881e-6 +
                    (-0.719819512446612410046871622485e-7 +
                     (0.123861813967482187149578571399e-7 +
                      (-0.217520395756280036045213062393e-8 +
                       (0.388781809759014410074807442869e-9 +
                        (-0.700796265038755490062409794969e-10 +
                         (0.123354858659154476661143284704e-10 +
                          (-0.199154720339166450701114580768e-11 +
                           (0.272324468492957942440892325956e-12 +
                            (-0.290332466669136911144579608975e-13 +
                             (0.220377469939621954125626067392e-14 +
                              (-0.104427587851387682923206247345e-15 +
                               0.230360340483404522044679012564e-17 * x) *
                                  x) *
                                 x) *
                                x) *
                               x) *
                              x) *
                             x) *
                            x) *
                           x) *
                          x) *
                         x) *
                        x) *
                       x) *
                      x) *
                     x) *
                    x) *
                   x) *
                  x) *
                 x) *
            x);
  }

  // Now we merely need a way to calculate Gamma(x) from [4 , 1.0e6]
  return lgamma_1to2(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_LGAMMA_POSITIVE_H__
