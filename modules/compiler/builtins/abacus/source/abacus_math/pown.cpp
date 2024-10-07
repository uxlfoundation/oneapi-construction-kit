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
#ifdef __CA_BUILTINS_HALF_SUPPORT
#include <abacus/abacus_integer.h>
#endif  // __CA_BUILTINS_HALF_SUPPORT

#include <abacus/internal/floor_unsafe.h>
#include <abacus/internal/horner_polynomial.h>
#ifdef __CA_BUILTINS_HALF_SUPPORT
#include <abacus/internal/ldexp_unsafe.h>
#endif  // __CA_BUILTINS_HALF_SUPPORT
#include <abacus/internal/log2_extended_precision.h>
#include <abacus/internal/multiply_extended_precision.h>
#include <abacus/internal/trunc_unsafe.h>

namespace {
#ifdef __CA_BUILTINS_HALF_SUPPORT
// 2^x for range 0.0 -> 1.0, see pow.sollya for derivation
static ABACUS_CONSTANT abacus_half __codeplay_pown_unsafe_coeffH[6] = {
    1.0f16,
    0.693359375f16,
    0.2384033203125f16,
    6.0699462890625e-2f16,
    3.490447998046875e-3f16,
    4.05120849609375e-3f16};
#endif  // __CA_BUILTINS_HALF_SUPPORT

static ABACUS_CONSTANT abacus_float __codeplay_pown_coeff[6] = {
    0.999999925066056f,    0.693153073167932f,    0.240153617206963f,
    0.558263175864784e-1f, 0.898934063766142e-2f, 0.187757646702639e-2f};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
// see maple worksheet for coefficient derivation
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

template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct helper<T, abacus_half> {
  using IntVecType =
      typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type;
  using SignedType = typename TypeTraits<T>::SignedType;
  using Shape = FPShape<T>;

  static T _(const T x, const IntVecType n) {
    // Calculate sign of final answer
    const SignedType n_is_odd = abacus::detail::cast::convert<SignedType>(
        IntVecType(n & 0x1) == IntVecType(0x1));
    const T ans_is_negative = __abacus_select(
        T(1.0f16), T(-1.0f16), SignedType(n_is_odd & __abacus_signbit(x)));
    const T xAbs = __abacus_fabs(x);

    // Get a really precise log2(x) here
    T hiExp, loExp, log2X_hi, log2X_lo;
    if (__abacus_isftz()) {
      // To avoid denormal numbers in log2X_lo we return it as normalized
      // number with exponent in loExp. log2(x) is therefore represented as
      // components `(hiExp + log2X_hi + (log2X_lo * 2^loExp))`
      log2X_hi = abacus::internal::log2_extended_precision_half_safe(
          xAbs, &log2X_lo, &hiExp, &loExp);
    } else {
      // log2(x) returned as components `(hiExp + log2X_hi + log2X_lo)`,
      // where log2X_lo may be denormal
      log2X_hi = abacus::internal::log2_extended_precision_half_unsafe(
          xAbs, &log2X_lo, &hiExp);
    }

    // Deal with int multiplication overflow issues:
    const T n_float_cast = abacus::detail::cast::convert<T>(n);
    const T out_of_bounds = n_float_cast * (log2X_hi + hiExp);

    // Not all 32-bit integer values of n can be represented in half precision.
    // Retain the lost precision here. We calculate 'exp2(remainder * log2(x))'
    // in lower accuracy and multiply that with the result of 'exp2(n*log2(x)'.
    T n_remainder = abacus::detail::cast::convert<T>(
        n - abacus::detail::cast::convert<IntVecType>(n_float_cast));

    // Now we have an accurate log2(x) in the form hiExp + log2X_hi + log2X_lo
    // We want to multiply this accurately by n:
    T mul1_lo;
    T mul1_hi = abacus::internal::multiply_exact(hiExp, n_float_cast, &mul1_lo);
    const T mul1_remainder = hiExp * n_remainder;

    const SignedType cond_zero = hiExp == 0.0f16;
    mul1_hi = __abacus_select(mul1_hi, 0.0f16, cond_zero);
    mul1_lo = __abacus_select(mul1_lo, 0.0f16, cond_zero);

    // n can be too big for the Veltkamp_split to work correctly, as you
    // multiply n by 64.0 in it. Deal with this by scaling things
    const SignedType vk_split_bound =
        abacus::detail::cast::convert<SignedType>(__abacus_abs(n) > 64);
    T n_scaled = __abacus_select(
        n_float_cast, n_float_cast * 0.015625f16 /* 1/64 */, vk_split_bound);
    n_remainder = __abacus_select(
        n_remainder, n_remainder * 0.015625f16 /* 1/64 */, vk_split_bound);
    log2X_hi = __abacus_select(log2X_hi, log2X_hi * 64.0f16, vk_split_bound);

    // Now this should be fine:
    T mul2_lo;
    T mul2_hi = abacus::internal::multiply_exact(log2X_hi, n_scaled, &mul2_lo);
    const T mul2_remainder = log2X_hi * n_remainder;

    // Don't need the lo bits for log2X_lo * n:
    T mul3_hi = log2X_lo * n_float_cast;
    if (__abacus_isftz()) {
      // If we've normalized log2X_lo bring the exponent back after the
      // multiply, as the larger magnitude after the operation means applying
      // the exponent to scale the value will no longer result in a denormal.
      mul3_hi = abacus::internal::ldexp_unsafe(mul3_hi, loExp);
    }

    T mul3_remainder = log2X_lo * n_remainder;
    if (__abacus_isftz()) {
      // If we've normalized log2X_lo bring the exponent back after the
      // multiply, as the larger magnitude after the operation means applying
      // the exponent to scale the value will no longer result in a denormal.
      mul3_remainder = abacus::internal::ldexp_unsafe(mul3_remainder, loExp);
    }

    // All these mul's added together now give a really good approximation of
    // n * log2(x). Add them up as exactly as we can since we'll be exp2'ing at
    // the end to get our answer, we can subtract off the integer parts of these
    // numbers and ldexp that in at the end. This keeps our summation errors
    // lower, as all the values are being added in the same range
    // The rest of the stuff following, up to the polynomial, is trying to add
    // these numbers accurately
    SignedType exp_ans = abacus::detail::cast::convert<SignedType>(
        __abacus_floor(mul1_hi) + __abacus_floor(mul2_hi));

    mul1_hi = mul1_hi - __abacus_floor(mul1_hi);
    mul2_hi = mul2_hi - __abacus_floor(mul2_hi);
    T n_times_log2X = (mul1_hi + mul2_hi) + (mul1_lo + mul2_lo) + mul3_hi;

    // Get the integer bit of this and put it into expAns instead
    exp_ans += abacus::detail::cast::convert<SignedType>(
        __abacus_floor(n_times_log2X));
    n_times_log2X = n_times_log2X - __abacus_floor(n_times_log2X);

    // Now a normal exp2 should do the trick:
    // We know that 0 <= n_times_log2X <= 1 so we can just use a polynomial
    T result = abacus::internal::horner_polynomial(
        n_times_log2X, __codeplay_pown_unsafe_coeffH);

    // We do the same trick as for __log2_extra_precision here, using some extra
    // precision in the last few steps
    result = abacus::internal::ldexp_unsafe(result, exp_ans);

    // Take the remainder into account for the final result, since by maths
    // rules exp(n + remainder) ==> exp(n) * exp(remainder)
    const T remainder_times_log2X =
        mul1_remainder + mul2_remainder + mul3_remainder;
    const T remainder_poly = abacus::internal::horner_polynomial(
        remainder_times_log2X, __codeplay_pown_unsafe_coeffH);
    result *= remainder_poly;

    // Set correct sign for answer
    result *= ans_is_negative;

    result = __abacus_select(result, T(0.0f16),
                             SignedType(out_of_bounds < -27.0f16));

    result = __abacus_select(result, ans_is_negative * T(ABACUS_INFINITY),
                             SignedType(out_of_bounds > 17.0f16));

    // N inputs that would otherwise be large enough to cross the out of
    // bounds threshold for finite non-zero values of x are still defined when
    // x is 1.
    result =
        __abacus_select(result, ans_is_negative, SignedType(xAbs == 1.0f16));

    const SignedType x_is_zero =
        abacus::detail::cast::as<SignedType>(xAbs) == 0;
    const SignedType inf_cond =
        abacus::detail::cast::convert<SignedType>(n > 0) ^ x_is_zero;

    const T bit = ans_is_negative *
                  __abacus_select(T(0.0f16), T(ABACUS_INFINITY), inf_cond);
    result =
        __abacus_select(result, bit, SignedType(x_is_zero | __abacus_isinf(x)));

    result = __abacus_select(result, FPShape<T>::NaN(),
                             SignedType(__abacus_isnan(x)));

    // pown(x, 0) is 1 for any x, even zero, NaN or infinity.
    const SignedType n_is_zero =
        abacus::detail::cast::convert<SignedType>(n == 0);
    result = __abacus_select(result, T(1.0f16), n_is_zero);

    return result;
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct helper<T, abacus_float> {
  using SignedType = typename TypeTraits<T>::SignedType;
  using UnsignedType = typename TypeTraits<T>::UnsignedType;

  static T _(const T x, const SignedType n) {
    const T ans_is_negative = __abacus_select(
        (T)1.0f, -1.0f, ((n & 0x1) == 0x1) & __abacus_signbit(x));

    const T xAbs = __abacus_fabs(x);

    // get the exponent of x and other stuff
    SignedType xExp;
    T xMant = __abacus_frexp(xAbs, &xExp);

    const SignedType cond = xMant < 0.666666f;

    xMant = __abacus_select(xMant, xMant * 2.0f, cond);
    xExp = __abacus_select(xExp, xExp - 1, cond);

    // handier to have xExp as a float. Represented exactly
    const T xExp_float = abacus::detail::cast::convert<T>(xExp);

    // get 2 floats that sum to log2(xMant) very accurately
    T log2_lo;
    const T log2_hi =
        abacus::internal::log2_extended_precision(xMant, &log2_lo);

    // deal with int multiplication overflow issues:
    const T out_of_bounds =
        abacus::detail::cast::convert<T>(n) * (log2_hi + xExp_float);

    // The result is exp2( n*(xExp + log2_hi + log2_lo) )
    // We need the floor and mantissa of this value very accurately:
    SignedType exponent_floor;
    const T exponent_mantissa = abacus::internal::multiply_extended_precision(
        log2_hi, log2_lo, xExp_float, n, &exponent_floor);

    // 2^x from 0 -> 1
    T result = abacus::internal::horner_polynomial(exponent_mantissa,
                                                   __codeplay_pown_coeff);

    result = __abacus_ldexp(result, exponent_floor);

    result *= ans_is_negative;

    result = __abacus_select(result, 0.0f, out_of_bounds < -150.0f);

    result = __abacus_select(result, ans_is_negative * ABACUS_INFINITY,
                             out_of_bounds > 130.0f);

    const T bit = ans_is_negative * __abacus_select((T)0.0f, ABACUS_INFINITY,
                                                    (n > 0) ^ (x == 0.0f));
    result = __abacus_select(result, bit, (x == 0.0f) | __abacus_isinf(x));

    result = __abacus_select(result, FPShape<T>::NaN(), __abacus_isnan(x));

    result = __abacus_select(result, 1.0f, n == 0);

    return result;
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct helper<T, abacus_double> {
  using IntVecType =
      typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type;
  using SignedType = typename TypeTraits<T>::SignedType;

  static T _(const T x, const IntVecType n) {
    const T xAbs = __abacus_fabs(x);

    // get the exponent of x and other stuff
    IntVecType xExp;
    T xMant = __abacus_frexp(xAbs, &xExp);

    const abacus_double sqrt1Over2 = 7.07106769084930419921875e-1;
    const SignedType cond1 = xMant < sqrt1Over2;

    xMant = __abacus_select(xMant, xMant * 2.0, cond1);
    xExp = __abacus_select(xExp, xExp - 1,
                           abacus::detail::cast::convert<IntVecType>(cond1));

    // handier to have xExp as a float. Represented exactly
    const T xExp_float = abacus::detail::cast::convert<T>(xExp);

    // get 2 floats that sum to log2(xMant) very accurately
    T log2_lo;
    const T log2_hi =
        abacus::internal::log2_extended_precision(xMant, &log2_lo);

    // The result is exp2( n*(xExp + log2_hi + log2_lo) )
    // We need the floor and mantissa of this value very accurately:
    SignedType exponent_floor = 0;

    const T nAs = abacus::detail::cast::convert<T>(n);

    T xMant_times_y_hi_lo;
    T xMant_times_y_hi_hi =
        abacus::internal::multiply_exact(log2_hi, nAs, &xMant_times_y_hi_lo);

    T xExp_times_y_lo;
    T xExp_times_y_hi =
        abacus::internal::multiply_exact(xExp_float, nAs, &xExp_times_y_lo);

    const T xMant_times_y_lo_hi = nAs * log2_lo;

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

    const SignedType trunced_exponent_mantissa =
        abacus::internal::trunc_unsafe(exponent_mantissa);
    exponent_floor += trunced_exponent_mantissa;
    exponent_mantissa -=
        abacus::detail::cast::convert<T>(trunced_exponent_mantissa);

    T result = (T)1.0 + (exponent_mantissa *
                         abacus::internal::horner_polynomial(
                             exponent_mantissa, __codeplay_pow_unsafe_coeffD));

    result = __abacus_ldexp(
        result, abacus::detail::cast::convert<IntVecType>(exponent_floor));

    const SignedType n_odd =
        abacus::detail::cast::convert<SignedType>((n & 0x1) == 0x1);

    const SignedType negativeCond = n_odd & __abacus_signbit(x);
    const T ans_is_negative = __abacus_select((T)1.0, (T)-1.0, negativeCond);

    result = result * ans_is_negative;

    // deal with int multiplication overflow issues:
    const T out_of_bounds = nAs * (log2_hi + xExp_float);

    const SignedType cond2 = out_of_bounds > 1025.0;
    result = __abacus_select(result, ans_is_negative * ABACUS_INFINITY, cond2);

    const SignedType cond3 = out_of_bounds < -1080.0;
    result = __abacus_select(result, (T)0.0, cond3);

    const SignedType furtherCond1 =
        abacus::detail::cast::convert<SignedType>(n > 0) ^ (x == 0.0);
    const T furtherBit1 =
        __abacus_select((T)0.0, (T)ABACUS_INFINITY, furtherCond1);

    const SignedType cond5 = (x == 0.0) | __abacus_isinf(x);
    result = __abacus_select(result, ans_is_negative * furtherBit1, cond5);

    const SignedType cond6 = __abacus_isnan(x);
    result = __abacus_select(result, x, cond6);

    const SignedType cond7 = abacus::detail::cast::convert<SignedType>(n == 0);
    result = __abacus_select(result, (T)1.0, cond7);

    return result;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
inline T pown(
    const T x,
    const typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type &n) {
  return helper<T>::_(x, n);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_pown(abacus_half x, abacus_int n) {
  return pown<>(x, n);
}
abacus_half2 ABACUS_API __abacus_pown(abacus_half2 x, abacus_int2 n) {
  return pown<>(x, n);
}
abacus_half3 ABACUS_API __abacus_pown(abacus_half3 x, abacus_int3 n) {
  return pown<>(x, n);
}
abacus_half4 ABACUS_API __abacus_pown(abacus_half4 x, abacus_int4 n) {
  return pown<>(x, n);
}
abacus_half8 ABACUS_API __abacus_pown(abacus_half8 x, abacus_int8 n) {
  return pown<>(x, n);
}
abacus_half16 ABACUS_API __abacus_pown(abacus_half16 x, abacus_int16 n) {
  return pown<>(x, n);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_pown(abacus_float x, abacus_int n) {
  return pown<>(x, n);
}
abacus_float2 ABACUS_API __abacus_pown(abacus_float2 x, abacus_int2 n) {
  return pown<>(x, n);
}
abacus_float3 ABACUS_API __abacus_pown(abacus_float3 x, abacus_int3 n) {
  return pown<>(x, n);
}
abacus_float4 ABACUS_API __abacus_pown(abacus_float4 x, abacus_int4 n) {
  return pown<>(x, n);
}
abacus_float8 ABACUS_API __abacus_pown(abacus_float8 x, abacus_int8 n) {
  return pown<>(x, n);
}
abacus_float16 ABACUS_API __abacus_pown(abacus_float16 x, abacus_int16 n) {
  return pown<>(x, n);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_pown(abacus_double x, abacus_int n) {
  return pown<>(x, n);
}
abacus_double2 ABACUS_API __abacus_pown(abacus_double2 x, abacus_int2 n) {
  return pown<>(x, n);
}
abacus_double3 ABACUS_API __abacus_pown(abacus_double3 x, abacus_int3 n) {
  return pown<>(x, n);
}
abacus_double4 ABACUS_API __abacus_pown(abacus_double4 x, abacus_int4 n) {
  return pown<>(x, n);
}
abacus_double8 ABACUS_API __abacus_pown(abacus_double8 x, abacus_int8 n) {
  return pown<>(x, n);
}
abacus_double16 ABACUS_API __abacus_pown(abacus_double16 x, abacus_int16 n) {
  return pown<>(x, n);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
