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

#ifndef __ABACUS_INTERNAL_POW_UNSAFE_H__
#define __ABACUS_INTERNAL_POW_UNSAFE_H__

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/floor_unsafe.h>
#ifdef __CA_BUILTINS_HALF_SUPPORT
#include <abacus/internal/ldexp_unsafe.h>
#endif  // __CA_BUILTINS_HALF_SUPPORT
#include <abacus/internal/log2_extended_precision.h>
#include <abacus/internal/multiply_exact.h>
#include <abacus/internal/multiply_exact_unsafe.h>
#include <abacus/internal/trunc_unsafe.h>

namespace {
#ifdef __CA_BUILTINS_HALF_SUPPORT
// 2^x for range [0.0, 1.0], see pow.sollya for derivation
static ABACUS_CONSTANT abacus_half __codeplay_pow_unsafe_coeffH[6] = {
    1.0f16,
    0.693359375f16,
    0.2384033203125f16,
    6.0699462890625e-2f16,
    3.490447998046875e-3f16,
    4.05120849609375e-3f16};
#endif  // __CA_BUILTINS_HALF_SUPPORT

// 2^x for range [0.0, 1.0], see pow.sollya for derivation
static ABACUS_CONSTANT abacus_float __codeplay_pow_unsafe_coeff[8] = {
    1.0f,
    0.693147182464599609375f,
    0.24022643268108367919921875f,
    5.550487339496612548828125e-2f,
    9.61467809975147247314453125e-3f,
    1.341356779448688030242919921875e-3f,
    1.44149686093442142009735107421875e-4f,
    2.13267994695343077182769775390625e-5f};

// (2^x-1)/x for range [-0.5, 0.5], see powdouble.mw maple worksheet
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
static ABACUS_CONSTANT abacus_double __codeplay_pow_unsafe_coeffD[18] = {
    .69314718055994530941723217733,     .24022650695910071233355095749,
    0.55504108664821579953133084736e-1, 0.96181291076284771619935813805e-2,
    0.13333558146428443425886462306e-2, 0.15403530393381609934453551592e-3,
    0.15252733804059837717391762265e-4, 0.13215486790144321743443768087e-5,
    1.0178086009241027247693636286e-7,  7.0549116207971902311700693112e-9,
    4.4455382714849808951628664454e-10, 2.5678436000477774056278194713e-11,
    1.3691489511954971230628762315e-12, 6.7787256843162869264041638867e-14,
    3.1323713565579919861469734507e-15, 1.3570535861859933139810634493e-16,
    5.5680060148351051509171469418e-18, 2.1306677337585862223671406870e-19};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
}  // namespace

namespace abacus {
namespace internal {
template <typename T>
struct IntFloatPart {
  T float_part;
  typename TypeTraits<T>::SignedType int_part;
};

template <typename T>
IntFloatPart<T> reduction(const T &x) {
  IntFloatPart<T> result;
  result.int_part = abacus::internal::trunc_unsafe(x);
  result.float_part = x - abacus::detail::cast::convert<T>(result.int_part);
  return result;
}

template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct pow_unsafe_helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct pow_unsafe_helper<T, abacus_half> {
  static T _(const T &x, const T &y) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    // Get a really precise log2(x) here
    T hiExp, loExp, log2X_hi, log2X_lo;
    if (__abacus_isftz()) {
      // To avoid denormal numbers in log2X_lo we return it as normalized
      // number with exponent in loExp. log2(x) is therefore represented as
      // components `(hiExp + log2X_hi + (log2X_lo * 2^loExp))`
      log2X_hi = abacus::internal::log2_extended_precision_half_safe(
          x, &log2X_lo, &hiExp, &loExp);
    } else {
      // log2(x) returned as components `(hiExp + log2X_hi + log2X_lo)`,
      // where log2X_lo may be denormal
      log2X_hi = abacus::internal::log2_extended_precision_half_unsafe(
          x, &log2X_lo, &hiExp);
    }

    // Deal with overflow issues
    const T overflow_check = y * (hiExp + log2X_hi);

    // Now we have an accurate log2(x) in the form hiExp + log2X_hi + log2X_lo
    // We want to multiply this accurately by y:
    T mul1_lo;
    T mul1_hi = abacus::internal::multiply_exact(hiExp, y, &mul1_lo);

    const SignedType cond_zero = hiExp == 0.0f16;
    mul1_hi = __abacus_select(mul1_hi, 0.0f16, cond_zero);
    mul1_lo = __abacus_select(mul1_lo, 0.0f16, cond_zero);

    // y can be too big for the Veltkamp_split to work correctly, as you
    // multiply y by 64.0 in it. Deal with this by scaling things
    const SignedType vk_split_bound = __abacus_fabs(y) > 64.0f16;
    T y_scaled = __abacus_select(y, y * 0.015625f16 /* 1/64 */, vk_split_bound);
    log2X_hi = __abacus_select(log2X_hi, log2X_hi * 64.0f16, vk_split_bound);

    // Now this should be fine:
    T mul2_lo;
    T mul2_hi = abacus::internal::multiply_exact(log2X_hi, y_scaled, &mul2_lo);

    // Don't need the lo bits for log2X_lo * y:
    T mul3_hi = log2X_lo * y;
    if (__abacus_isftz()) {
      // If we've normalized log2X_lo bring the exponent back after the
      // multiply, as the larger magnitude after the operation means applying
      // the exponent to scale the value will no longer result in a denormal.
      mul3_hi = abacus::internal::ldexp_unsafe(mul3_hi, loExp);
    }

    // All these mul's added together now give a really good approximation of
    // y * log2(x). Add them up as exactly as we can since we'll be exp2'ing at
    // the end to get our answer, we can subtract off the integer parts of these
    // numbers and ldexp that in at the end. This keeps our summation errors
    // lower, as all the values are being added in the same range
    // The rest of the stuff following, up to the polynomial, is trying to add
    // these numbers accurately
    SignedType exp_ans = abacus::detail::cast::convert<SignedType>(
        __abacus_floor(mul1_hi) + __abacus_floor(mul2_hi));

    mul1_hi = mul1_hi - __abacus_floor(mul1_hi);
    mul2_hi = mul2_hi - __abacus_floor(mul2_hi);
    T y_times_log2X = (mul1_hi + mul2_hi) + (mul1_lo + mul2_lo) + mul3_hi;

    // Get the integer bit of this and put it into expAns instead
    exp_ans += abacus::detail::cast::convert<SignedType>(
        __abacus_floor(y_times_log2X));
    y_times_log2X = y_times_log2X - __abacus_floor(y_times_log2X);

    // Now a normal exp2 should do the trick:
    // We know that 0 <= y_times_log2X <= 1 so we can just use a polynomial
    T result = abacus::internal::horner_polynomial(
        y_times_log2X, __codeplay_pow_unsafe_coeffH);

    // We do the same trick as for __log2_extra_precision here, using some extra
    // precision in the last few steps
    result = abacus::internal::ldexp_unsafe(result, exp_ans);

    // Prevents edge case pow(HLF_MAX, 1.0) resulting in INFINITY
    result = __abacus_select(result, x, SignedType(y == 1.0f16));

    // Check for overflow values above bias + 2
    result = __abacus_select(result, T(ABACUS_INFINITY),
                             SignedType(overflow_check > 17.0f16));
    // Check for underflow under -(bias + mant bits + 2)
    result =
        __abacus_select(result, 0.0f16, SignedType(overflow_check < -27.0f16));

    return result;
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct pow_unsafe_helper<T, abacus_float> {
  static T _(const T &x, const T &y) {
    typedef typename TypeTraits<T>::SignedType SignedType;
    typedef typename TypeTraits<T>::UnsignedType UnsignedType;

    // get the exponent of x and other stuff
    SignedType xExp;
    T xMant = __abacus_frexp(x, &xExp);

    const SignedType cond = xMant <= 0.671092f;
    xMant = __abacus_select(xMant, xMant * 2.0f, cond);
    xExp = __abacus_select(xExp, xExp - 1, cond);

    // handier to have xExp as a float. Represented exactly
    T xExp_float = abacus::detail::cast::convert<T>(xExp);

    // get 2 floats that sum to log2(xMant) very accurately
    T log2_lo;
    T log2_hi = abacus::internal::log2_extended_precision(xMant, &log2_lo);

    // deal with overflow issues:
    const T out_of_bounds = y * (log2_hi + xExp_float);

    // The result is exp2( n*(xExp + log2_hi + log2_lo) )
    // We need the floor and mantissa of this value very accurately:
    SignedType exponent_floor = 0;

    T xMant_times_y_hi_lo;
    T xMant_times_y_hi_hi =
        abacus::internal::multiply_exact(log2_hi, y, &xMant_times_y_hi_lo);

    T xExp_times_y_lo;
    T xExp_times_y_hi =
        abacus::internal::multiply_exact(xExp_float, y, &xExp_times_y_lo);

    T xMant_times_y_lo_hi = y * log2_lo;

    // get the floor vals and subtract them off:
    exponent_floor += abacus::internal::trunc_unsafe(xMant_times_y_hi_hi);
    exponent_floor += abacus::internal::trunc_unsafe(xExp_times_y_hi);

    xMant_times_y_hi_hi -= abacus::detail::cast::convert<T>(
        abacus::internal::trunc_unsafe(xMant_times_y_hi_hi));
    xExp_times_y_hi -= abacus::detail::cast::convert<T>(
        abacus::internal::trunc_unsafe(xExp_times_y_hi));

    T exponent_mantissa = (xExp_times_y_hi + xMant_times_y_hi_hi) +
                          xExp_times_y_lo +
                          (xMant_times_y_lo_hi + xMant_times_y_hi_lo);

    exponent_floor += abacus::internal::floor_unsafe(exponent_mantissa);
    exponent_mantissa -= abacus::detail::cast::convert<T>(
        abacus::internal::floor_unsafe(exponent_mantissa));

    T result = abacus::internal::horner_polynomial(exponent_mantissa,
                                                   __codeplay_pow_unsafe_coeff);

    result = __abacus_ldexp(result, exponent_floor);

    result = __abacus_select(result, 0.0f, out_of_bounds < -150.0f);
    result = __abacus_select(result, ABACUS_INFINITY, out_of_bounds > 130.0f);

    const UnsignedType rUint = abacus::detail::cast::as<UnsignedType>(result);

    const SignedType fudge_direction = y < 0.0f;
    const T fudged = __abacus_select(
        abacus::detail::cast::as<T>(rUint - UnsignedType(1)),
        abacus::detail::cast::as<T>(rUint + UnsignedType(1)), fudge_direction);

    result = __abacus_select(fudged, result, (result == 0.0f) | isinf(result));
    return result;
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct pow_unsafe_helper<T, abacus_double> {
  static T _(const T &x, const T &y) {
    typedef typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type
        IntVecType;
    typedef typename TypeTraits<T>::SignedType SignedType;
    typedef typename TypeTraits<T>::UnsignedType UnsignedType;

    // get the exponent of x and other stuff
    IntVecType xExp;
    T xMant = __abacus_frexp(x, &xExp);

    // log2 is harder to calculate the further you get away from 0. xMant is in
    // the range [0.5..1.0), and we want to push this value into a range around
    // 1.0, which corresponds to a result from the log2 calculation of around
    // 0.0. The 'done' way to do this is to use sqrt(0.5) as the bound check,
    // but we've found (through trial and error) that values that fell in the
    // range [0.6839..sqrt(0.5)] were likely to develop higher ULP errors that
    // propogated through the remaining arithmetic. Lowering the bound means we
    // catch more values that are edge case are promote them to a range we can
    // more accurately calculate with.
    const SignedType cond1 = xMant < 0.68399999999999999999999999999999999999;
    xMant = __abacus_select(xMant, xMant * 2.0, cond1);
    xExp = __abacus_select(xExp, xExp - 1,
                           abacus::detail::cast::convert<IntVecType>(cond1));

    // handier to have xExp as a float. Represented exactly
    const T xExp_float = abacus::detail::cast::convert<T>(xExp);

    // get 2 floats that sum to log2(xMant) very accurately
    T log2_lo;
    const T log2_hi =
        abacus::internal::log2_extended_precision(xMant, &log2_lo);

    // The result is exp2( y*(xExp + log2_hi + log2_lo) )
    // We need the floor and mantissa of this value very accurately:
    SignedType exponent_floor = 0;

    T xMant_times_y_hi_lo;
    T xMant_times_y_hi_hi = abacus::internal::multiply_exact_unsafe(
        log2_hi, y, &xMant_times_y_hi_lo);

    T xExp_times_y_lo;
    T xExp_times_y_hi = abacus::internal::multiply_exact_unsafe(
        xExp_float, y, &xExp_times_y_lo);

    T xMant_times_y_lo_hi = y * log2_lo;

    const IntFloatPart<T> xMant_times_y_hi_hi_reduced =
        reduction(xMant_times_y_hi_hi);
    const IntFloatPart<T> xExp_times_y_hi_reduced = reduction(xExp_times_y_hi);

    xMant_times_y_hi_hi = xMant_times_y_hi_hi_reduced.float_part;
    exponent_floor += xMant_times_y_hi_hi_reduced.int_part;

    xExp_times_y_hi = xExp_times_y_hi_reduced.float_part;
    exponent_floor += xExp_times_y_hi_reduced.int_part;

    T exponent_mantissa = (xMant_times_y_hi_hi + xExp_times_y_hi) +
                          xExp_times_y_lo +
                          (xMant_times_y_lo_hi + xMant_times_y_hi_lo);

    const SignedType mantissa_trunc =
        abacus::internal::trunc_unsafe(exponent_mantissa);
    exponent_floor += mantissa_trunc;
    exponent_mantissa -= abacus::detail::cast::convert<T>(mantissa_trunc);

    T result = (T)1.0 + (exponent_mantissa *
                         abacus::internal::horner_polynomial(
                             exponent_mantissa, __codeplay_pow_unsafe_coeffD));

    result = __abacus_ldexp(
        result, abacus::detail::cast::convert<IntVecType>(exponent_floor));

    const UnsignedType resultUint =
        abacus::detail::cast::as<UnsignedType>(result);
    const SignedType cond2 = y < 0.0;
    const T fudgeFactor = abacus::detail::cast::as<T>(
        __abacus_select(resultUint - 3, resultUint + 3, cond2));

    const UnsignedType resultAbsUint = resultUint & 0x7fffffffffffffff;
    const SignedType cond3 =
        (resultAbsUint <= 0x7FEFFFFFFFFFFFFC) & (resultAbsUint >= 0x3);
    result = __abacus_select(result, fudgeFactor, cond3);

    // deal with overflow issues:
    const T out_of_bounds = y * (log2_hi + xExp_float);

    const SignedType cond4 = out_of_bounds > 1026.0;
    result = __abacus_select(result, (T)ABACUS_INFINITY, cond4);

    const SignedType cond5 = out_of_bounds < -3000.0;
    result = __abacus_select(result, (T)0.0, cond5);

    return result;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

// Used for for x >= 0.0 via the identity pow(x, y) = exp2(y * log2(x))
template <typename T>
inline T pow_unsafe(const T &x, const T &y) {
  return pow_unsafe_helper<T>::_(x, y);
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_POW_UNSAFE_H__
