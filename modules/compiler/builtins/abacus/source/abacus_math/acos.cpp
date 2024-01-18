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

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
#include <abacus/internal/atan_unsafe.h>
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
#include <abacus/internal/horner_polynomial.h>
#include <abacus/internal/sqrt.h>
#ifdef __CA_BUILTINS_HALF_SUPPORT
#include <abacus/internal/add_exact.h>
#include <abacus/internal/multiply_exact.h>
#endif

// find the interval x is in with 4 comparisons.
static ABACUS_CONSTANT abacus_float intervals[16] = {
    ABACUS_INFINITY, 0.999f, 0.998f, 0.97f, 0.95f, 0.9f,  0.835f, 0.77f,
    0.74f,           0.71f,  0.69f,  0.6f,  0.48f, 0.39f, 0.28f,  0.15f};

// see maple worksheet for how polynomial was derived
static ABACUS_CONSTANT abacus_float polynomial[64] = {0.0f,
                                                      2.0000000000e+00f,
                                                      3.3333331347e-01f,
                                                      8.8923707604e-02f,
                                                      .3320429300e-11f,
                                                      1.99999987050413f,
                                                      .333559900590094f,
                                                      0.0f,
                                                      0.0f,
                                                      2.00000018983691f,
                                                      0.333300550232606f,
                                                      0.0906289393516710f,
                                                      -7.0323446538e-8f,
                                                      2.00000729697439f,
                                                      .333053153313176f,
                                                      0.935903556451160e-1f,
#ifdef __CODEPLAY_RTZ__
                                                      0.0f,
                                                      2.00001880188406f,
                                                      0.332665942561604f,
                                                      0.0966426106268719f,
#else
                                                      0.0f,
                                                      2.0000119970933942631f,
                                                      .33282879809945984175f,
                                                      0.95684772740795236624e-1f,
#endif
                                                      -0.974483389899e-5f,
                                                      2.00030227284687f,
                                                      .329881521685121f,
                                                      .105883422218206f,
                                                      -0.670371162773e-4f,
                                                      2.00129593729056f,
                                                      .324076676217644f,
                                                      .117307260206189f,
                                                      .217639271718735f,
                                                      2.71336747094746f,
                                                      -3.50975863061689f,
                                                      2.95123389137115f,
                                                      .2305110745f,
                                                      2.564410557f,
                                                      -2.934845643f,
                                                      2.211182622f,
                                                      -0.3386731455429e-3f,
                                                      2.00447557435642f,
                                                      .311554004290407f,
                                                      .133905165066045f,
                                                      -0.7697614965471e-3f,
                                                      2.00847672264655f,
                                                      .299164928458703f,
                                                      .146704246497234f,
                                                      -0.2608680584536e-2f,
                                                      2.02195155643835f,
                                                      .266169966167320f,
                                                      .173705996465032f,
                                                      1.57944478530369f,
                                                      -1.07213487856054f,
                                                      .210327897559596f,
                                                      -.390062463027004f,
                                                      1.57257672304095f,
                                                      -1.01973591420641f,
                                                      0.76723539217937e-1f,
                                                      -.276208425813826f,
                                                      1.57094520703110f,
                                                      -1.00269496683898f,
                                                      0.16990556445836e-1f,
                                                      -.205915089538965f,
                                                      1.57079644927736f,
                                                      -1.00002275042913f,
                                                      0.6451722120450e-3f,
                                                      -.171670615589947f};

abacus_float ABACUS_API __abacus_acos(abacus_float x) {
  abacus_float xAbs = __abacus_fabs(x);

  const abacus_int high_eight = (xAbs <= intervals[8]) ? 8 : 0;
  const abacus_int high_four =
      high_eight + ((xAbs <= intervals[high_eight + 4]) ? 4 : 0);
  const abacus_int high_two =
      high_four + ((xAbs <= intervals[high_four + 2]) ? 2 : 0);
  const abacus_int interval =
      high_two + ((xAbs <= intervals[high_two + 1]) ? 1 : 0);

  // if x is close to one we need more accuracy so we use 1-x (if 0.5 < x < 1.0
  // this subtraction is exact)
  xAbs = (interval < 12) ? 1.0f - xAbs : xAbs;

// approximate acos*acos:
#ifdef __CODEPLAY_USE_ESTRIN_POLYNOMIAL_REDUCTION__
  abacus_float ans = __Codeplay__estrin_4coeff(xAbs, polynomial + interval * 4);
#else
  const abacus_float ans =
      abacus::internal::horner_polynomial(xAbs, polynomial + interval * 4, 4);
#endif

  // get acos:
  abacus_float result = ans;
  if (interval < 12 && interval != 8 && interval != 7) {
    result = abacus::internal::sqrt(ans);
  }

  return (x > 0.0f) ? result : ABACUS_PI_F - result;
}

namespace {
template <typename T>
T acos(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;
  typedef typename TypeTraits<UnsignedType>::ElementType UnsignedElementType;
  const T xAbs = __abacus_fabs(x);

  UnsignedType interval = 0;
  T ans = 0.0f;

  const T oneMinusXAbs = (T)1.0f - xAbs;
  for (UnsignedElementType i = 0; i < 16; i++) {
    const SignedType cond = xAbs <= intervals[i];
    interval = __abacus_select(interval, i, cond);

    const T poly = abacus::internal::horner_polynomial(
        i < 12 ? oneMinusXAbs : xAbs, polynomial + i * 4, 4);

    ans = __abacus_select(ans, poly, cond);
  }

  T result =
      __abacus_select(ans, abacus::internal::sqrt(ans),
                      (interval < 12) & (interval != 8) & (interval != 7));

  result = __abacus_select((T)ABACUS_PI_F - result, result, x > 0);

  return __abacus_select(result, ABACUS_NAN, __abacus_isnan(x));
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT

namespace {
// The same polynomial used in asin(x)
static ABACUS_CONSTANT abacus_half __codeplay_acos_1[3] = {
    -2.0f16, 0.330810546875f16, -0.1080322265625f16};

static ABACUS_CONSTANT abacus_half __codeplay_acos_2[2] = {-0.99853515625f16,
                                                           -0.1995849609375f16};

template <typename T>
T ABACUS_API acos_half(T x) {
  // This implementation is very like asin(x), as the two function only really
  // differ mathematically by a constant. See asin(x) for more details
  typedef typename TypeTraits<T>::SignedType SignedType;

  T xAbs = __abacus_fabs(x);

  const SignedType xBig = (xAbs > T(5.9375E-1f16));

  const T x2 = x * x;

  // Calculate horner polynomial by hand, making use of multiply_exact to
  // increase resulting precision.
  T mul_lo;
  const T mul_hi =
      abacus::internal::multiply_exact<T>(x2, __codeplay_acos_2[1], &mul_lo);
  // For all possible inputs of acos, we have tested that exponent of
  // __codeplay_acos_2[0] is >= expoonent of mul_hi, so therefore it's safe to
  // use add_exact instead of add_exact_safe.
  T mul_add_lo;
  const T mul_add_hi =
      abacus::internal::add_exact<T>(__codeplay_acos_2[0], mul_hi, &mul_add_lo);
  mul_add_lo = mul_add_lo + mul_lo;

  // Multiply by xAbs.
  T mul_abs_lo;
  const T mul_abs_hi =
      abacus::internal::multiply_exact<T>(mul_add_hi, xAbs, &mul_abs_lo);
  mul_abs_lo = mul_abs_lo + mul_add_lo * xAbs;

  T ans = mul_abs_hi + mul_abs_lo + ABACUS_PI_2_H;

  xAbs = xAbs - T(1.0f16);
  T ansBig =
      xAbs * abacus::internal::horner_polynomial(xAbs, __codeplay_acos_1);

  ansBig = abacus::internal::sqrt(ansBig);

  ans = __abacus_select(ans, ansBig, xBig);
  ans = __abacus_select(ans, ABACUS_PI_H - ans, SignedType(x < 0.0f16));

  return ans;
}
}  // namespace

abacus_half ABACUS_API __abacus_acos(abacus_half x) { return acos_half<>(x); }
abacus_half2 ABACUS_API __abacus_acos(abacus_half2 x) { return acos_half<>(x); }
abacus_half3 ABACUS_API __abacus_acos(abacus_half3 x) { return acos_half<>(x); }
abacus_half4 ABACUS_API __abacus_acos(abacus_half4 x) { return acos_half<>(x); }
abacus_half8 ABACUS_API __abacus_acos(abacus_half8 x) { return acos_half<>(x); }
abacus_half16 ABACUS_API __abacus_acos(abacus_half16 x) {
  return acos_half<>(x);
}

#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float2 ABACUS_API __abacus_acos(abacus_float2 x) { return acos<>(x); }
abacus_float3 ABACUS_API __abacus_acos(abacus_float3 x) { return acos<>(x); }
abacus_float4 ABACUS_API __abacus_acos(abacus_float4 x) { return acos<>(x); }
abacus_float8 ABACUS_API __abacus_acos(abacus_float8 x) { return acos<>(x); }
abacus_float16 ABACUS_API __abacus_acos(abacus_float16 x) { return acos<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
namespace {
template <typename T>
T acosD(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  const T xAbs = __abacus_fabs(x);

  T result = abacus::internal::atan_unsafe(__abacus_sqrt((T)1 - x) /
                                           __abacus_sqrt((T)1 + x)) *
             2;

  // x == -1.0, result = PI. x == 1.0, result = 0 (with some trickery)
  const SignedType cond = xAbs == 1.0;
  result = __abacus_select(
      result, (T)ABACUS_PI_2 - __abacus_copysign((T)ABACUS_PI_2, x), cond);

  return result;
}
}  // namespace

abacus_double ABACUS_API __abacus_acos(abacus_double x) { return acosD<>(x); }
abacus_double2 ABACUS_API __abacus_acos(abacus_double2 x) { return acosD<>(x); }
abacus_double3 ABACUS_API __abacus_acos(abacus_double3 x) { return acosD<>(x); }
abacus_double4 ABACUS_API __abacus_acos(abacus_double4 x) { return acosD<>(x); }
abacus_double8 ABACUS_API __abacus_acos(abacus_double8 x) { return acosD<>(x); }
abacus_double16 ABACUS_API __abacus_acos(abacus_double16 x) {
  return acosD<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
