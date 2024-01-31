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
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/horner_polynomial.h>

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
#include <abacus/internal/atan_unsafe.h>
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
#include <abacus/internal/sqrt.h>

// see maple worksheet for how coefficients were derived.
static ABACUS_CONSTANT abacus_float __codeplay_asin_coeff[80] = {
    0.0f,
    -2.00004375220900f,
    .0f,
    .0f,
    .0f,
    -3.20351451e-15f,
    -2.0000000000383029f,
    .33333322170212445f,
    -0.88991738600893899e-1f,
    .0f,
    1.0502694416942e-12f,
    -1.9999999991782101f,
    .33333353114553749f,
    -0.88870120200849796e-1f,
    0.29313562922267067e-1f,
    -1.47921547154e-7f,
    -2.00001315023359f,
    .332909299388145f,
    -.947425206302640e-1f,
    .0f,
    -3.7847223913e-7f,
    -2.00002818985907f,
    .332600219160040f,
    -.967458727609798e-1f,
    .0f,
    -0.867969648723e-5f,
    -2.00027459677420f,
    .330117458668047f,
    -.105223200149336f,
    .0f,
    0.2273354322721701784e-5f,
    -1.9999351176855053392f,
    .33406722838951698648f,
    -0.84803575479616734909e-1f,
    0.39538985315315980244e-1f,
    0.2202132810520088070e-4f,
    -1.9995886763791310109f,
    .33636897583419322503f,
    -0.77933953113237608489e-1f,
    0.47314837601091224670e-1f,
    0.8398867991759355953e-4f,
    -1.9987710465294802801f,
    .34042559243602761827f,
    -0.68963851635638730663e-1f,
    0.54773972163077802810e-1f,
    0.569795072078704833984375e-1f,
    .55090773105621337890625f,
    1.35681283473968505859375f,
    -1.7243263721466064453125f,
    1.0800836086273193359375f,
    0.10083685629069805145263671875e-1f,
    .9011461734771728515625f,
    .374265968799591064453125f,
    -.4971446096897125244140625f,
    .50429713726043701171875f,
    0.202080910094082355499267578125e-2f,
    .9751389026641845703125f,
    .11918354034423828125f,
    -.105579674243927001953125f,
    .27844560146331787109375f,
    0.363773462595418095588684082031e-3f,
    .99407970905303955078125f,
    0.37745825946331024169921875e-1f,
    0.505259148776531219482421875e-1f,
    .16588188707828521728515625f,
    0.357234566763509064912796020508e-4f,
    .99912166595458984375f,
    0.8507016114890575408935546875e-2f,
    .126389205455780029296875f,
    0.91543965041637420654296875e-1f,
    0.0f,
    0.999987900257110595703125f,
    0.000453866057796403765678405761719f,
    0.1604931056499481201171875f,
    0.0359851531684398651123046875f,
    0.0f,
    1.0f,
    0.476546938443789258599281311035e-5f,
    .16625429689884185791015625f,
    0.10135334916412830352783203125e-1f};

// find the interval x is in with 4 comparisons. Note if x >0.995 it's in
// interval 0, and 0.0f is in interval 15. (kinda backwards)
static ABACUS_CONSTANT abacus_float intervals[16] = {
    ABACUS_INFINITY, 0.9999f, 0.998f, 0.97f, 0.93f, 0.895f, 0.85f, 0.77f,
    0.71f,           0.62f,   0.53f,  0.42f, 0.35f, 0.25f,  0.16f, 0.07f};

abacus_float ABACUS_API __abacus_asin(abacus_float x) {
  if (__abacus_isinf(x)) {
    return __abacus_copysign(ABACUS_NAN, x);
  }
  const abacus_float xAbs = __abacus_fabs(x);

  const int high_eight = (xAbs < intervals[8]) ? 8 : 0;
  const int high_four =
      high_eight + ((xAbs < intervals[high_eight + 4]) ? 4 : 0);
  const int high_two = high_four + ((xAbs < intervals[high_four + 2]) ? 2 : 0);
  const int interval = high_two + ((xAbs < intervals[high_two + 1]) ? 1 : 0);

  float ans = (interval < 9) ? xAbs - 1.0f : xAbs;

#ifdef __CODEPLAY_USE_ESTRIN_POLYNOMIAL_REDUCTION__
  ans = __Codeplay__estrin_5coeff(ans, __codeplay_asin_coeff + interval * 5);
#else
  ans = abacus::internal::horner_polynomial(
      ans, __codeplay_asin_coeff + interval * 5, 5);
#endif

  if (interval < 9) {
    ans = -abacus::internal::sqrt(ans);
    ans += ABACUS_PI_2_F;
  }

  return x > 0 ? ans : -ans;
}

#ifdef __CA_BUILTINS_HALF_SUPPORT

namespace {
static ABACUS_CONSTANT abacus_half __codeplay_asin_1[3] = {
    -2.0f16, 0.330810546875f16, -0.1080322265625f16};

static ABACUS_CONSTANT abacus_half __codeplay_asin_2[3] = {
    1.0f16, 0.1629638671875f16, 0.10528564453125f16};

template <typename T>
T asin_half(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  T xAbs = __abacus_fabs(x);

  // around x = 1 we want to estimate (asin(x) - pi/2)^2
  SignedType xBig = (xAbs > T(5.9375E-1f16));
  const T x2 = x * x;
  T ans = x * abacus::internal::horner_polynomial(x2, __codeplay_asin_2);

  xAbs = xAbs - T(1.0f16);

  T ansBig =
      xAbs * abacus::internal::horner_polynomial(xAbs, __codeplay_asin_1);

  ansBig = -abacus::internal::sqrt(ansBig) + ABACUS_PI_2_H;
  ansBig = __abacus_copysign(ansBig, x);

  ans = __abacus_select(ans, ansBig, xBig);

  return ans;
}

template <>
abacus_half asin_half(const abacus_half x) {
  abacus_half xAbs = __abacus_fabs(x);

  // Around x = 1 we want to estimate (asin(x) - pi/2)^2 instead of asin(x)
  // This is due to the slope of asin(x) aproaching infinity as is gets closer
  // to 1, so no polynomial can really estimate it.
  // From the expansion of asin(x) about 1
  // (https://www.wolframalpha.com/input/?i=expansion+asin(x)+about+1) we can
  // see that it's got a factor of sqrt(x - 1) in every term that can be taken
  // out to give a normal polynomial.
  // This is suggestive that we expand out (asin(x) - asin(1))^2 about 1
  // instead, and indeed this gives a more normal expansion
  // (https://www.wolframalpha.com/input/?i=expansion+(asin(x)+-+asin(1))%5E2+about+1)
  // So we know it should be good

  if (xAbs > 0.59375f16) {
    xAbs = xAbs - 1.0f16;

    // Estimate (asin(x + 1) - pi / 2)^2 (see solly script)
    abacus_half ans =
        xAbs * abacus::internal::horner_polynomial(xAbs, __codeplay_asin_1);

    ans = -abacus::internal::sqrt(ans) + ABACUS_PI_2_H;
    return __abacus_copysign(ans, x);
  }

  // Estimate the remaining values
  const abacus_half x2 = x * x;
  return x * abacus::internal::horner_polynomial(x2, __codeplay_asin_2);
}
}  // namespace

#endif  // __CA_BUILTINS_HALF_SUPPORT

namespace {
template <typename T>
T asin(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;
  typedef typename TypeTraits<UnsignedType>::ElementType UnsignedElementType;
  const T xAbs = __abacus_fabs(x);

  UnsignedType interval = 0;
  T ans = 0.0f;

  const T oneMinusXAbs = xAbs - 1.0f;
  for (UnsignedElementType i = 0; i < 16; i++) {
    const SignedType cond = xAbs < intervals[i];
    interval = __abacus_select(interval, i, cond);

    const T poly = abacus::internal::horner_polynomial(
        i < 9 ? oneMinusXAbs : xAbs, __codeplay_asin_coeff + i * 5, 5);

    ans = __abacus_select(ans, poly, cond);
  }

  T result = __abacus_select(ans, -abacus::internal::sqrt(ans) + ABACUS_PI_2_F,
                             (interval < 9));

  result = __abacus_select(-result, result, x > 0);

  return __abacus_select(result, __abacus_copysign(ABACUS_NAN, x),
                         __abacus_isinf(x) | __abacus_isnan(x));
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT

abacus_half ABACUS_API __abacus_asin(abacus_half x) { return asin_half<>(x); }
abacus_half2 ABACUS_API __abacus_asin(abacus_half2 x) { return asin_half<>(x); }
abacus_half3 ABACUS_API __abacus_asin(abacus_half3 x) { return asin_half<>(x); }
abacus_half4 ABACUS_API __abacus_asin(abacus_half4 x) { return asin_half<>(x); }
abacus_half8 ABACUS_API __abacus_asin(abacus_half8 x) { return asin_half<>(x); }
abacus_half16 ABACUS_API __abacus_asin(abacus_half16 x) {
  return asin_half<>(x);
}

#endif  //__CA_BUILTINS_HALF_SUPPORT

abacus_float2 ABACUS_API __abacus_asin(abacus_float2 x) { return asin<>(x); }
abacus_float3 ABACUS_API __abacus_asin(abacus_float3 x) { return asin<>(x); }
abacus_float4 ABACUS_API __abacus_asin(abacus_float4 x) { return asin<>(x); }
abacus_float8 ABACUS_API __abacus_asin(abacus_float8 x) { return asin<>(x); }
abacus_float16 ABACUS_API __abacus_asin(abacus_float16 x) { return asin<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
namespace {
template <typename T>
T asinD(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  T result =
      (T)2 * abacus::internal::atan_unsafe(
                 x / (__abacus_sqrt((T)1 - x) * __abacus_sqrt((T)1 + x) + 1));

  const SignedType cond1 = __abacus_fabs(x) == 1.0;
  result = __abacus_select(result, __abacus_copysign(ABACUS_PI_2, x), cond1);

  return result;
}
}  // namespace

abacus_double ABACUS_API __abacus_asin(abacus_double x) { return asinD<>(x); }
abacus_double2 ABACUS_API __abacus_asin(abacus_double2 x) { return asinD<>(x); }
abacus_double3 ABACUS_API __abacus_asin(abacus_double3 x) { return asinD<>(x); }
abacus_double4 ABACUS_API __abacus_asin(abacus_double4 x) { return asinD<>(x); }
abacus_double8 ABACUS_API __abacus_asin(abacus_double8 x) { return asinD<>(x); }
abacus_double16 ABACUS_API __abacus_asin(abacus_double16 x) {
  return asinD<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
