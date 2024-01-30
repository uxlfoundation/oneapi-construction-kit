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
#include <abacus/internal/is_denorm.h>

// see maple worksheet for coefficient derivation
static ABACUS_CONSTANT abacus_float __codeplay_asinh_coeff[80] = {
    -1.0f,
    1.0f,
    .0f,
    .0f,
    .0f,
    -.9841130040f,
    1.999813427f,
    .1008208092e-5f,
    -2.514212483e-9f,
    2.333132365e-12f,
    -.9464972061f,
    1.997776238f,
    .4483950320e-4f,
    -4.387494631e-7f,
    1.668579525e-9f,
    -.8878891608f,
    1.990027366f,
    .4394061227e-3f,
    -.9586834447e-5f,
    8.285510739e-8f,
    -.8075797168f,
    1.970479968f,
    .2249371043e-2f,
    -.8504079201e-4f,
    .1275701225e-5f,
    -.6825967168f,
    1.918856504f,
    .1035027348e-1f,
    -.6567618285e-3f,
    .1656470574e-4f,
    -.5096741248f,
    1.801687772f,
    .4046664810e-1f,
    -.4134537368e-2f,
    .1686534816e-3f,
    -.3303513156f,
    1.614945911f,
    .1139784305f,
    -.1709812409e-1f,
    .1032518601e-2f,
    -.1716427691f,
    1.378300867f,
    .2471121562f,
    -0.5058659688e-1f,
    0.4209608984e-2f,
    -0.4895680885e-1f,
    1.120498865f,
    .4516319125f,
    -.1231819683f,
    0.1393596186e-1f,
    -0.106557798e-2f,
    .9800959071f,
    .6066854211f,
    -.1996540845f,
    0.2815186734e-1f,
    -0.1205735994e-1f,
    1.067182543f,
    -.1388301966f,
    -0.5727839843e-1f,
    0.2235704336e-1f,
    -.1871158369e-2f,
    1.013151706f,
    -.307486718e-1f,
    -.1539192738f,
    .5494659535e-1f,
    .71292949e-4f,
    .9986730189f,
    .994582290e-2f,
    -.2050774270f,
    .7923602379e-1f,
    .141070171e-4f,
    .9996082939f,
    .428855155e-2f,
    -.1900015948f,
    .6425963867e-1f,
    .0f,
    .9999998831f,
    .314622946e-4f,
    -.1679851869f,
    .1772655414e-1f};

static ABACUS_CONSTANT abacus_float intervals[16] = {ABACUS_INFINITY,
                                                     400.0f,
                                                     80.0f,
                                                     30.0f,
                                                     17.0f,
                                                     10.0f,
                                                     6.0f,
                                                     3.8f,
                                                     2.7f,
                                                     1.9f,
                                                     1.3f,
                                                     1.0f,
                                                     .75f,
                                                     .5f,
                                                     .3f,
                                                     .12f};

abacus_float ABACUS_API __abacus_asinh(abacus_float x) {
  if (__abacus_isftz() && abacus::internal::is_denorm(x)) {
    return x;
  }

  if (__abacus_isinf(x)) {
    return x;
  }

  if (__abacus_isnan(x)) {
    return x;
  }

  const abacus_float xAbs = __abacus_fabs(x);

  const abacus_int high_eight = (xAbs < intervals[8]) ? 8 : 0;
  const abacus_int high_four =
      high_eight + ((xAbs < intervals[high_eight + 4]) ? 4 : 0);
  const abacus_int high_two =
      high_four + ((xAbs < intervals[high_four + 2]) ? 2 : 0);
  const abacus_int interval =
      high_two + ((xAbs < intervals[high_two + 1]) ? 1 : 0);

  abacus_float ans = abacus::internal::horner_polynomial(
      xAbs, __codeplay_asinh_coeff + interval * 5, 5);

  if (interval < 11) {
    ans = __abacus_log1p(ans);
  }

  if (interval == 0) {
    // multiply by ln(2)
    ans += 0.6931476593017578125f;
  }

  return __abacus_copysign(ans, x);
}

namespace {
template <typename T>
T asinh(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;
  typedef typename TypeTraits<UnsignedType>::ElementType UnsignedElementType;

  const T xAbs = __abacus_fabs(x);

  UnsignedType interval = 0;
  T ans = 0.0f;

  for (UnsignedElementType i = 0; i < 16; i++) {
    const SignedType cond = xAbs < intervals[i];
    interval = __abacus_select(interval, i, cond);

    const T poly = abacus::internal::horner_polynomial(
        xAbs, __codeplay_asinh_coeff + i * 5, 5);

    ans = __abacus_select(ans, poly, cond);
  }

  T result = __abacus_select(ans, __abacus_log1p(ans), interval < 11);

  // multiply by ln(2)
  result =
      __abacus_select(result, result + 0.6931476593017578125f, interval == 0);

  result = __abacus_copysign(result, x);

  return __abacus_select(result, x, __abacus_isinf(x) | __abacus_isnan(x));
}
}  // namespace

abacus_float2 ABACUS_API __abacus_asinh(abacus_float2 x) { return asinh<>(x); }
abacus_float3 ABACUS_API __abacus_asinh(abacus_float3 x) { return asinh<>(x); }
abacus_float4 ABACUS_API __abacus_asinh(abacus_float4 x) { return asinh<>(x); }
abacus_float8 ABACUS_API __abacus_asinh(abacus_float8 x) { return asinh<>(x); }
abacus_float16 ABACUS_API __abacus_asinh(abacus_float16 x) {
  return asinh<>(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
namespace {
/*
  first off use the identity asinh(-x) = -asinh(x) to remove negative numbers

  we have four distinct ranges where our problem differs;

  a) x = [0.0 .. 1.0842021724855045e-015]
  b) x = (1.0842021724855045e-015 .. 0.6]
  c) x = (0.6 .. 3.6028797e+16)
  d) x = [3.6028797e+16 .. +infinity)

  For each range, we have a different solution;

  a) asinh(x) = x
  b) asinh(x) = x * (asinh(k) / k) - where k = sqrt(x * x). We use a 14-degree
     polynomial to solve this range
  c) asinh(x) = log(x + sqrt(x * x + 1)), the inverse hyperbolic identity
  d) asinh(x) = log(x) + log(2)
*/
template <typename T>
T asinhD(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  const T xAbs = __abacus_fabs(x);
  const abacus_double ln2 =
      0.693147180559945309417232121458176568075500134360255254120680;

  const SignedType large = xAbs > 3.6028797e+16;
  const T toLog = __abacus_select(xAbs + __abacus_sqrt((T)1.0 + (xAbs * xAbs)),
                                  xAbs, large);

  const T logged = __abacus_log(toLog);

  const T afterLog = __abacus_select(logged, logged + ln2, large);

  const abacus_double polynomial[14] = {
      0.9999999999999999972206401e0,  -0.1666666666666635872050374e0,
      0.7499999999943372697244269e-1, -0.4464285710178115466437382e-1,
      0.3038194288611832212289934e-1, -0.2237212356062910993191083e-1,
      0.1735223711132927243798608e-1, -0.1395949498044520097428285e-1,
      0.1151359840974553405511583e-1, -0.9566271955764454782914346e-2,
      0.7670591834281017654139093e-2, -0.5403578695078777809556689e-2,
      0.2818764122836144912311135e-2, -0.7716999562952834196266079e-3};

  const T poly = x * abacus::internal::horner_polynomial(x * x, polynomial);

  const SignedType small = xAbs <= 0.6;
  T result = __abacus_select(afterLog, poly, small);

  const SignedType tiny = xAbs <= 1.0842021724855045e-015;
  result = __abacus_select(result, x, tiny);

  return __abacus_copysign(result, x);
}
}  // namespace

abacus_double ABACUS_API __abacus_asinh(abacus_double x) { return asinhD<>(x); }
abacus_double2 ABACUS_API __abacus_asinh(abacus_double2 x) {
  return asinhD<>(x);
}
abacus_double3 ABACUS_API __abacus_asinh(abacus_double3 x) {
  return asinhD<>(x);
}
abacus_double4 ABACUS_API __abacus_asinh(abacus_double4 x) {
  return asinhD<>(x);
}
abacus_double8 ABACUS_API __abacus_asinh(abacus_double8 x) {
  return asinhD<>(x);
}
abacus_double16 ABACUS_API __abacus_asinh(abacus_double16 x) {
  return asinhD<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

#ifdef __CA_BUILTINS_HALF_SUPPORT
namespace {
// See asinh sollya script for derivations
static ABACUS_CONSTANT abacus_half _asinhH1[6] = {
    0.8818359375f16,         0.70751953125f16,        0.172119140625f16,
    -3.06549072265625e-2f16, -8.58306884765625e-3f16, 2.948760986328125e-3f16};
static ABACUS_CONSTANT abacus_half _asinhH2[2] = {0.99951171875f16,
                                                  -0.148193359375f16};

// See scalar implementation for more details about the algorithm.
template <typename T>
T asinhH(const T x) {
  using SignedType = typename TypeTraits<T>::SignedType;

  const T xBigBound = 10.0f16;
  const T xOverflowBound = 32768.0f16;

  const T xAbs = __abacus_fabs(x);
  const T sign = __abacus_copysign(T(1.0f16), x);

  // A small optimization for vectorized versions. Rather than call __abacus_log
  // a second time to handle the log(2x) case, we instead do a smaller branch to
  // pick the input value into log:
  const T log_input =
      __abacus_select(xAbs, xAbs * 2.0f16,
                      SignedType(xAbs >= xBigBound && xAbs < xOverflowBound));
  const T logX = __abacus_log(log_input);

  // Since we have to calculate log(x) anyway, we may as well use it:
  T ans = abacus::internal::horner_polynomial(logX, _asinhH1);

  // Logically, this case should be log(2x). However, to avoid calling
  // __abacus_log twice, we change the input of __abacus_log above to be 2x,
  // therefore 'logX' ends up being log(2x).
  ans = sign * __abacus_select(ans, logX, SignedType(xAbs >= xBigBound));

  ans = __abacus_select(ans, sign * (ABACUS_LN2_H + logX),
                        SignedType(xAbs >= xOverflowBound));
  ans = __abacus_select(
      ans, x * abacus::internal::horner_polynomial(x * x, _asinhH2),
      SignedType(xAbs < 0.55f16));

  // When denormals are unavailable, we need to handle the smallest FP16 value
  // explicitly (6.103515625e-05).
  ans = __abacus_select(
      ans, x,
      SignedType(SignedType(__abacus_isftz()) && xAbs == 6.103515625e-05));

  return ans;
}

template <>
abacus_half asinhH(const abacus_half x) {
  const abacus_half xAbs = __abacus_fabs(x);

  // When denormals are unavailable, we need to handle the smallest FP16 value
  // explicitly (6.103515625e-05).
  if (__abacus_isftz() && xAbs == 6.103515625e-05) {
    return x;
  }

  // With the usual definition of asinh(x) = log(x + sqrt(x^2 + 1)) we get all
  // sorts of precision/cancellation issues for smaller numbers. These resolve
  // themselves for fabs(x) > 0.55.
  // For values less than this we use a direct polynomial to get the answer:
  if (xAbs < 0.55f16) {
    return x * abacus::internal::horner_polynomial(x * x, _asinhH2);
  }

  const abacus_half sign = __abacus_copysign(1.0f16, x);
  const abacus_half logX = __abacus_log(xAbs);

  // As |x| > 10, asinh(x) = log(x + sqrt(x^2 + 1)) converges to
  // asinh(x) = log(2x), as the + 1 becomes insignificant.
  //
  // However, for inputs >= 32768, 2 * x will overflow, so we use
  // asinh(x) = log(2) + log(x) instead. Note that we can't use this for
  // |x| > 10, as this is not precise enough.
  if (xAbs >= 32768.0f16) {
    return sign * (ABACUS_LN2_H + logX);
  } else if (xAbs >= 10.0f16) {
    return sign * __abacus_log(xAbs * 2.0f16);
  }

  // Since we have to calculate log(x) anyway, we may as well use it:
  const abacus_half ans = abacus::internal::horner_polynomial(logX, _asinhH1);

  return sign * ans;
}
}  // namespace

abacus_half ABACUS_API __abacus_asinh(abacus_half x) { return asinhH<>(x); }
abacus_half2 ABACUS_API __abacus_asinh(abacus_half2 x) { return asinhH<>(x); }
abacus_half3 ABACUS_API __abacus_asinh(abacus_half3 x) { return asinhH<>(x); }
abacus_half4 ABACUS_API __abacus_asinh(abacus_half4 x) { return asinhH<>(x); }
abacus_half8 ABACUS_API __abacus_asinh(abacus_half8 x) { return asinhH<>(x); }
abacus_half16 ABACUS_API __abacus_asinh(abacus_half16 x) { return asinhH<>(x); }
#endif  // __CA_BUILTINS_HALF_SUPPORT
