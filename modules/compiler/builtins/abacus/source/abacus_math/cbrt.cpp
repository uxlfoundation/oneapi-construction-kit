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
#include <abacus/abacus_misc.h>
#include <abacus/abacus_relational.h>
#include <abacus/internal/horner_polynomial.h>
#include <abacus/internal/is_denorm.h>

namespace {
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T, typename U>
T shuffle_helper(const abacus_double4 t, const U u) {
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;
  return __abacus_shuffle(t, abacus::detail::cast::convert<UnsignedType>(u));
}

template <>
abacus_double shuffle_helper<abacus_double, abacus_uint>(const abacus_double4 t,
                                                         const abacus_uint u) {
  return t[u];
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct helper<T, abacus_half> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;
    typedef typename TypeTraits<T>::UnsignedType UnsignedType;

    // Since cbrt(-x) = -cbrt(x) we just calculate it for positive values and
    // copy the sign back in at the end.
    T xReduced = __abacus_fabs(x);

    // If x is too big or too small it messes up the intermediate calculations,
    // so in this case we run the algorithm on either x * 2^12 or x * 2^-12 to
    // bring it into good bounds, and at the end fix this by multiplying by 2^-4
    // or 2^4 respectively.
    const SignedType xSmall = abacus::internal::is_denorm(xReduced);
    const SignedType xBig =
        abacus::detail::cast::as<UnsignedType>(xReduced) > UnsignedType(0x7400);
    const SignedType xZero =
        abacus::detail::cast::as<UnsignedType>(xReduced) == UnsignedType(0);

    // Scale xSmall numbers by 4096(2^12)
    if (__abacus_isftz()) {
      // Scale denormal without using float multiply that could FTZ
      //
      // xUint | hiddenBit      Gives an exponent of -14
      // 4096                   2^12
      // 0.25                   2^-2
      //
      // exponent ==> -14 + 12 = -2
      const UnsignedType hiddenBit = FPShape<T>::LeastSignificantExponentBit();
      const UnsignedType xUint =
          abacus::detail::cast::as<UnsignedType>(xReduced);
      const T denorm_reduced =
          abacus::detail::cast::as<T>(UnsignedType(xUint | hiddenBit)) *
              4096.0f16 -
          0.25f16;
      xReduced = __abacus_select(xReduced, denorm_reduced, xSmall);
    } else {
      xReduced = __abacus_select(xReduced, xReduced * 4096.0f16, xSmall);
    }

    // 0.000244140625 = 2^-12
    xReduced = __abacus_select(xReduced, xReduced * 0.000244140625f16, xBig);

    // Similar to the sqrt algorithm, this involves a magic number hack and a
    // Newton-Rhapson iteration.
    // This initial guess is derived from
    // http://h14s.p5r.org/2012/09/0x5f3759df.html?mwh=1
    // This article gives a value of 0x27e1, however this seems to be the
    // optimal values for algorithms without a Newton-Rhapson iteration.
    // With one Newton-Rhapson iteration as below, by brute force computation
    // all values from 0x27db -> 0x27df seem to work better.
    // 0x27dd was chosen because, while any of the values in this range pass the
    // necessary 2-ulp test, 0x27dd gives the most values < 1.5 ulps.

    const T initialGuess = abacus::detail::cast::as<T>(UnsignedType(
        abacus::detail::cast::as<UnsignedType>(xReduced) / 3 + 0x27dd));

    // One iteration of Newton-Raphson
    const T guessSqr = initialGuess * initialGuess;
    T ans = ((initialGuess * 2.0f16) + (xReduced / guessSqr)) * 0.333333333f16;

    // 16 = 2^4
    ans = __abacus_select(ans, ans * 16.0f16, xBig);

    // 0.0625 = 2^-4
    ans = __abacus_select(ans, ans * 0.0625f16, xSmall);

    // inf,nan,zero checks:
    ans = __abacus_select(
        ans, x, SignedType(__abacus_isinf(x) || __abacus_isnan(x) || xZero));

    // Use sign component from original input
    return __abacus_copysign(ans, x);
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct helper<T, abacus_float> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;
    typedef typename TypeTraits<T>::UnsignedType UnsignedType;

    const T xAbs = __abacus_fabs(x);

    T xReduced = xAbs;

    // 0x70000000 == 2^97
    const SignedType xBig =
        xAbs > abacus::detail::cast::as<T>(UnsignedType(0x70000000));
    // 0x18300000 == 2^-79
    const SignedType xSmall =
        xAbs < abacus::detail::cast::as<T>(UnsignedType(0x18300000));

    // 0x2f000000 == 2^-33
    xReduced = __abacus_select(
        xReduced,
        xReduced * abacus::detail::cast::as<T>(UnsignedType(0x2F000000)), xBig);

    // 0x5A800000 == 2^54
    xReduced = __abacus_select(
        xReduced,
        xReduced * abacus::detail::cast::as<T>(UnsignedType(0x5A800000)),
        xSmall);

    // Get a good initial guess for cbrt (using some magic!)
    // 0x2a517d3c == (2.0/3.0) * 2^23 * (127 - 0.0450465)
    const T initialGuess = abacus::detail::cast::as<T>(
        abacus::detail::cast::as<UnsignedType>(xReduced) / 3 + 0x2a517d3c);

    // one iteration of Newton-Raphson
    const T guessSqr = initialGuess * initialGuess;
    T ans = (initialGuess * 2.0f + xReduced / guessSqr) * 0.333333333f;

    // one iteration of Halley's method
    const T ansCbd = ans * ans * ans;
    ans = ans - (ansCbd - xReduced) * ans / (ansCbd * 2.0f + xReduced);

    // For denormal values the Halley's calculation can end up as NaN if ansCbd
    // gets flushed to zero. Fallback to zero for denormals since this result
    // can be within ULP error.
    if (__abacus_isftz()) {
      ans = __abacus_select(ans, 0.0f, ansCbd == 0.0f);
    }

    // 2048 == (2^33)^(1/3)
    ans = __abacus_select(ans, ans * 2048.0f, xBig);

    // 0x36800000 == 2^-18 = (2^54)^(1/3)
    ans = __abacus_select(
        ans, ans * abacus::detail::cast::as<T>(UnsignedType(0x36800000)),
        xSmall);

    // Use sign component from original input
    const T signedAns = __abacus_copysign(ans, x);

    // Return the original input value if x is +/- infinity or 0. Check for
    // denormals since FTZ can throw off the zero equality comparison.
    const SignedType copySign = (xAbs == 0.0f) | __abacus_isinf(x);
    if (__abacus_isftz()) {
      const SignedType xDenorm = abacus::internal::is_denorm(x);
      return __abacus_select(signedAns, x, copySign & ~xDenorm);
    }
    return __abacus_select(signedAns, x, copySign);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct helper<T, abacus_double> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;
    typedef typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type
        IntType;
    typedef typename MakeType<abacus_uint, TypeTraits<T>::num_elements>::type
        UintType;

    IntType xExp;
    T xMant = __abacus_fabs(__abacus_frexp(x, &xExp));

    IntType expAns = xExp / 3;
    // force expRemainder to always end up being positive
    expAns = __abacus_select(expAns, expAns - 1, xExp < 0);

    const IntType expRemainder = xExp - expAns * 3;

    const abacus_double polynomial[11] = {
        0.3016866403890285027141003e0,  0.2136093385352237667433857e1,
        -0.5411969778179924025377674e1, 0.1261942133026000409355824e2,
        -0.2233533231730374376663173e2, 0.2911072654436043025297143e2,
        -0.2745254040040803908250648e2, 0.1821530436624125677581295e2,
        -0.8067019738312171229138886e1, 0.2141417905436719198700902e1,
        -0.2577879379162866445639990e0};

    // estimate the cbrt here, xMant [0.5 -> 1]:
    T ans = abacus::internal::horner_polynomial(xMant, polynomial);

    abacus_double4 cbrts;
    // skip term
    cbrts[0] = 1.0;
    // cbrt(2)
    cbrts[1] = 1.259921049894873164767210607278228350570251464701507980081975;
    // cbrt(4)
    cbrts[2] = 1.587401051968199474751705639272308260391493327899853009808285;
    // cbrt(8)
    cbrts[3] = 2.0;

    ans *= shuffle_helper<T>(cbrts,
                             abacus::detail::cast::as<UintType>(expRemainder));
    xMant *= abacus::detail::cast::convert<T>((IntType)1 << expRemainder);

    // one convergence iteration
    const T ansCbd = ans * ans * ans;
    ans -= (ansCbd - xMant) * ans / (ansCbd * 2.0 + xMant);

    const T result = __abacus_copysign(__abacus_ldexp(ans, expAns), x);

    const SignedType cond = (x == 0.0) | (__abacus_isfinite(x) == 0);

    return __abacus_select(result, x, cond);
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T cbrt(const T x) {
  return helper<T>::_(x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_cbrt(abacus_half x) { return cbrt<>(x); }
abacus_half2 ABACUS_API __abacus_cbrt(abacus_half2 x) { return cbrt<>(x); }
abacus_half3 ABACUS_API __abacus_cbrt(abacus_half3 x) { return cbrt<>(x); }
abacus_half4 ABACUS_API __abacus_cbrt(abacus_half4 x) { return cbrt<>(x); }
abacus_half8 ABACUS_API __abacus_cbrt(abacus_half8 x) { return cbrt<>(x); }
abacus_half16 ABACUS_API __abacus_cbrt(abacus_half16 x) { return cbrt<>(x); }
#endif

abacus_float ABACUS_API __abacus_cbrt(abacus_float x) { return cbrt<>(x); }
abacus_float2 ABACUS_API __abacus_cbrt(abacus_float2 x) { return cbrt<>(x); }
abacus_float3 ABACUS_API __abacus_cbrt(abacus_float3 x) { return cbrt<>(x); }
abacus_float4 ABACUS_API __abacus_cbrt(abacus_float4 x) { return cbrt<>(x); }
abacus_float8 ABACUS_API __abacus_cbrt(abacus_float8 x) { return cbrt<>(x); }
abacus_float16 ABACUS_API __abacus_cbrt(abacus_float16 x) { return cbrt<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_cbrt(abacus_double x) { return cbrt<>(x); }
abacus_double2 ABACUS_API __abacus_cbrt(abacus_double2 x) { return cbrt<>(x); }
abacus_double3 ABACUS_API __abacus_cbrt(abacus_double3 x) { return cbrt<>(x); }
abacus_double4 ABACUS_API __abacus_cbrt(abacus_double4 x) { return cbrt<>(x); }
abacus_double8 ABACUS_API __abacus_cbrt(abacus_double8 x) { return cbrt<>(x); }
abacus_double16 ABACUS_API __abacus_cbrt(abacus_double16 x) {
  return cbrt<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
