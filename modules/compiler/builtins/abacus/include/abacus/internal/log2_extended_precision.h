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

#ifndef __ABACUS_INTERNAL_LOG2_EXTENDED_PRECISION_H__
#define __ABACUS_INTERNAL_LOG2_EXTENDED_PRECISION_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>
#include <abacus/internal/add_exact.h>
#include <abacus/internal/horner_polynomial.h>
#ifdef __CA_BUILTINS_HALF_SUPPORT
#include <abacus/internal/ldexp_unsafe.h>
#endif  // __CA_BUILTINS_HALF_SUPPORT
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
#include <abacus/internal/log_extended_precision.h>
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
#include <abacus/internal/multiply_exact.h>
#include <abacus/internal/multiply_exact_unsafe.h>

namespace {
#ifdef __CA_BUILTINS_HALF_SUPPORT
// Aprroximation of log2(x+1) between [-0.25;0.5]
// See log2_extended_precision.sollya for derivation
// In order to gain performance we've dropped the first term of the polynomial
// out, since it's in 32-bit precision and we'll manually add it in exactly by
// splitting into 16-bit values.
static constexpr ABACUS_CONSTANT abacus_float
    __codeplay_log2_extended_precision_coeffH0 = 1.442689418792724609375f;
static ABACUS_CONSTANT abacus_half
    __codeplay_log2_extended_precision_coeffH[5] = {
        -0.72119140625f16, 0.4814453125f16, -0.369384765625f16, 0.2919921875f16,
        -0.1474609375f16};
#endif  // __CA_BUILTINS_HALF_SUPPORT

// see maple worksheet for how coefficient was derived
static ABACUS_CONSTANT abacus_float
    __codeplay_log2_extended_precision_coeff[16] = {
        -.72134752044367709f,     .48089834695927680f,     -.36067376055899679f,
        .28853900903746190f,      -.24044913178516529f,    .20609921341711487f,
        -.18033920103731198f,     .16030301033157655f,     -.14420120724015226f,
        .13106324614753728f,      -.12135615255608116f,    .11232690718087393f,
        -0.92542744786243758e-1f, 0.84679695565939149e-1f, -.13981754775229332f,
        .13540255679513207f};
}  // namespace

namespace abacus {
namespace internal {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct log2_extended_precision_helper;

template <typename T>
struct log2_extended_precision_helper<T, abacus_float> {
  static T _(const T &xMant, T *out_remainder) {
    T xMAnt1m = xMant - 1.0f;

    T poly = abacus::internal::horner_polynomial(
        xMAnt1m, __codeplay_log2_extended_precision_coeff);

    T poly_times_x_lo;
    T poly_times_x_hi =
        abacus::internal::multiply_exact(poly, xMAnt1m, &poly_times_x_lo);

    T total_sum_lo;
    T total_sum_hi = abacus::internal::add_exact(
        (T)1.44269502162933349609375f, poly_times_x_hi, &total_sum_lo);

    total_sum_lo += poly_times_x_lo;
    // total_sum_lo += 1.837066650390625e-8f;  // a magic correction term
    total_sum_lo += 1.92596333e-8f;

    T result_lo;
    T result_hi =
        abacus::internal::multiply_exact(total_sum_hi, xMAnt1m, &result_lo);

    *out_remainder = result_lo + (xMAnt1m * total_sum_lo);
    return result_hi;
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct log2_extended_precision_helper<T, abacus_double> {
  static T _(const T &xMant, T *out_remainder) {
    // Get the natural log, then convert it to log2
    T log_lo;
    const T log_hi = abacus::internal::log_extended_precision(xMant, &log_lo);

    // an accurate 1/ln(2) (both needed)
    const T recip_ln2_hi =
        1.442695040888963387004650940070860087871551513671875;
    const T recip_ln2_lo =
        2.0355273740931032049555094440481110934135449406931109219181185079885526622893506345e-17;

    T log2_hi_lo;
    const T log2_hi_hi = abacus::internal::multiply_exact_unsafe(
        log_hi, recip_ln2_hi, &log2_hi_lo);

    *out_remainder =
        log2_hi_lo + (log_lo * recip_ln2_hi + log_hi * recip_ln2_lo);
    return log2_hi_hi;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
inline T log2_extended_precision(const T &xMant, T *out_remainder) {
  return log2_extended_precision_helper<T>::_(xMant, out_remainder);
}

#ifdef __CA_BUILTINS_HALF_SUPPORT
// Specialized half functions for use in pow/powr/pown because a normal log2
// doesn't give us enough accuracy for what we need, we have to use a
// specialised function for it.
//
// This returns 2 halfs, an ans and an ans_lo, with the sum of these two halfs
// giving the answer in extra precision.
//
// Unsafe differs from safe version in that ans_lo may be denormal, so should
// be used on devices that support denormal numbers.
template <typename T>
T log2_extended_precision_half_unsafe(const T &x, T *ans_lo, T *xExp) {
  using SignedType = typename TypeTraits<T>::SignedType;
  using UnsignedType = typename TypeTraits<T>::UnsignedType;
  using IntVecType =
      typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type;

  IntVecType xExpI;
  T xMant = __abacus_frexp(x, &xExpI);

  // Put XMant between  0.75 < xMant < 1.5
  const SignedType scale_cond = xMant < 0.75f16;
  xMant = __abacus_select(xMant, 2.0f16 * xMant, scale_cond);
  xExpI =
      __abacus_select(xExpI, IntVecType(xExpI - 1),
                      abacus::detail::cast::convert<IntVecType>(scale_cond));

  // Approximate log2(x+1) with polynomial
  xMant = xMant - 1.0f16;
  T poly_start = abacus::internal::horner_polynomial(
      xMant, __codeplay_log2_extended_precision_coeffH);

  T poly_lo;
  T poly_hi = abacus::internal::multiply_exact(xMant, poly_start, &poly_lo);

  // In place exactly add in the single precision term which we dropped from
  // the start of our sollya polynomial solution as two half components
  // 1.442689418792724609375 = 1.4423828125 + 0.000306606292724609375:
  T c_term_hi = 1.4423828125f16;
  T c_term_lo = 0.000306606292724609375f16;

  abacus::internal::add_exact(&c_term_hi, &poly_hi);
  // This adds in exactly, so no need for an add_exact
  c_term_lo = c_term_lo + poly_lo;

  abacus::internal::add_exact(&c_term_lo, &poly_hi);
  abacus::internal::add_exact(&c_term_hi, &c_term_lo);

  // This adds in exactly, no need for add_exact
  c_term_lo = c_term_lo + poly_hi;

  // We now know, through exhaustive checking, that the original sum of
  // (1.44269275665283203125 + xMant * poly_start) is now exactly contained as
  // (c_term_hi + c_term_lo)
  // So we now only need xMant * (c_term_hi + c_term_lo) as precisely as
  // possible
  T final_mul_lo;
  const T final_mul_hi =
      abacus::internal::multiply_exact(xMant, c_term_hi, &final_mul_lo);

  // Final answer
  T ans = final_mul_hi;

  // Don't need the low bits of (xMant * c_term_lo)
  T remainder = final_mul_lo + (xMant * c_term_lo);

  // Single awkward boundary value fix:
  const SignedType edge(abacus::detail::cast::as<UnsignedType>(x) == 0x39f6);
  remainder = __abacus_select(remainder, T(-0.000144362f16), edge);

  // Set return parameters
  *xExp = abacus::detail::cast::convert<T>(xExpI);
  *ans_lo = remainder;

  ans = __abacus_select(ans, T(-ABACUS_INFINITY), SignedType(0.0f16 == x));
  ans = __abacus_select(ans, x, SignedType(__abacus_isinf(x)));
  return ans;
}

// Differs from the unsafe version in that it avoids returning denormal numbers
// in ans_lo, so should be used on devices without denormal support.
//
// Instead the returned ans_lo is a normalized mantissa with it's exponent
// returned via `loExp`.
template <typename T>
T log2_extended_precision_half_safe(const T &x, T *ans_lo, T *hiExp, T *loExp) {
  using SignedType = typename TypeTraits<T>::SignedType;
  using UnsignedType = typename TypeTraits<T>::UnsignedType;
  using IntVecType =
      typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type;

  IntVecType hiExpI;
  T xMant = __abacus_frexp(x, &hiExpI);

  // Put XMant between  0.75 < xMant < 1.5
  const SignedType scale_cond = xMant < 0.75f16;
  xMant = __abacus_select(xMant, 2.0f16 * xMant, scale_cond);
  hiExpI =
      __abacus_select(hiExpI, IntVecType(hiExpI - 1),
                      abacus::detail::cast::convert<IntVecType>(scale_cond));

  // Approximate log2(x+1) with polynomial
  xMant = xMant - 1.0f16;

  T poly_start = abacus::internal::horner_polynomial(
      xMant, __codeplay_log2_extended_precision_coeffH);

  // Avoid creating denormal numbers in the lo components of exact add and
  // multiply invocations by scaling xMant up by 2^5, as we've put xMant in
  // the range [0.75, 1.5] this shouldn't result in overflow. 5 was chosen as
  // the scale exponent as we'll multiply by xMant twice, resulting in an
  // overall scaling of 2^10. As there are 10 mantissa bits in half this will
  // mean that we cover all the denormal cases and scale them to a normal.
  const SignedType upscale_exp = 5;
  xMant = abacus::internal::ldexp_unsafe(xMant, upscale_exp);

  T poly_lo;
  T poly_hi = abacus::internal::multiply_exact(xMant, poly_start, &poly_lo);

  // In place exactly add in the single precision term which we dropped from
  // the start of our sollya polynomial solution as two half components
  // 1.442689418792724609375 = 1.4423828125 + 0.000306606292724609375:

  // 46.15625 ==> 1.4423828125 * 2^5
  T c_term_hi = 46.15625f16;

  // 0.0098114 ==> 0.000306606292724609375 * 2^5
  T c_term_lo = 0.0098114f16;

  abacus::internal::add_exact(&c_term_hi, &poly_hi);
  // This adds in exactly, so no need for an add_exact
  c_term_lo = c_term_lo + poly_lo;

  abacus::internal::add_exact(&c_term_lo, &poly_hi);
  abacus::internal::add_exact(&c_term_hi, &c_term_lo);

  // This adds in exactly, no need for add_exact
  c_term_lo = c_term_lo + poly_hi;

  // We now know, through exhaustive checking, that the original sum of
  // (1.44269275665283203125 + xMant * poly_start) is now exactly contained as
  // (c_term_hi + c_term_lo)
  // So we now only need xMant * (c_term_hi + c_term_lo) as precisely as
  // possible

  T final_mul_lo;
  T final_mul_hi =
      abacus::internal::multiply_exact(xMant, c_term_hi, &final_mul_lo);

  // Downscale final result by double the scale factor, since we've multiplied
  // by xMant twice.
  const SignedType downscale_exp = -10;
  final_mul_hi = abacus::internal::ldexp_unsafe(final_mul_hi, downscale_exp);

  // Final answer
  T ans = final_mul_hi;

  // Don't need the low bits of (xMant * c_term_lo)
  T remainder = final_mul_lo + (xMant * c_term_lo);

  // Single awkward boundary value fix:
  const SignedType edge(abacus::detail::cast::as<UnsignedType>(x) == 0x39f6);
  // -0.14783 ==> -0.000144362 * 2^10
  remainder = __abacus_select(remainder, T(-0.14783f16), edge);

  // Set return parameters
  *hiExp = abacus::detail::cast::convert<T>(hiExpI);

  // Split our remainder into a normalized mantissa and exponent, offsetting
  // the exponent by our scale factor.
  IntVecType loExpI;
  *ans_lo = __abacus_frexp(remainder, &loExpI);
  *loExp = abacus::detail::cast::convert<T>(
      loExpI + abacus::detail::cast::convert<IntVecType>(downscale_exp));

  ans = __abacus_select(ans, T(-ABACUS_INFINITY), SignedType(0.0f16 == x));
  ans = __abacus_select(ans, x, SignedType(__abacus_isinf(x)));
  return ans;
}

#endif  // __CA_BUILTINS_HALF_SUPPORT

/*
If you ever need a more accurate log2 for use in pow this is one. I wrote it
when tests were failing in the other building but ended up fixing it a different
way.
inline float log_extended_precision(float xMant, float * out_remainder){
        //New plan, get ln(xMAnt) first, then change to log2:
        float xMant1m = xMant - 1.0f;

        float poly = (-0.25f + (0.2f + (-0.16666573839961348e0f +
(0.14285549680165117e0f + (-0.12508235072240584e0f + (0.11122317723555506e0f +
(-0.97786162288840440e-1f + (0.88356744995684893e-1f + (-0.10603185816023552e0f
+ 0.10021792733479427e0f * xMant1m) * xMant1m) * xMant1m) * xMant1m) * xMant1m)
* xMant1m) * xMant1m) * xMant1m) *xMant1m) * xMant1m;
        poly += 3.333333432674407958984375e-1f;
        poly -= 9.934107462565104166666e-9; //To conensate for 1/3 not being
exact

        //We need xMant1m*(xMant1m*(xMant1m*poly - 0.5) + 1) accurately now

        //xMant1m*poly - 0.5:
        float term1_lo;
        float term1_hi = abacus::internal::multiply_exact(poly, xMant1m,
&term1_lo);

        float mhalf = -0.5f;
        abacus::internal::add_exact(&mhalf, &term1_hi);
        abacus::internal::add_exact(&term1_hi, &term1_lo);

        term1_lo = term1_hi;
        term1_hi = mhalf;

        //multiply by xMant1m:
        float term2_lo;
        float term2_hi = abacus::internal::multiply_exact(term1_hi, xMant1m,
&term2_lo);

        float term2_lo_lo;
        float term2_lo_hi = abacus::internal::multiply_exact(term1_lo, xMant1m,
&term2_lo_lo);

        //Make sure all these get added up ok:
        abacus::internal::add_exact(&term2_lo, &term2_lo_hi);
        abacus::internal::add_exact(&term2_hi, &term2_lo);

        abacus::internal::add_exact(&term2_lo_hi, &term2_lo_lo); //exact
        abacus::internal::add_exact(&term2_lo, &term2_lo_hi);
        abacus::internal::add_exact(&term2_hi, &term2_lo);


        //Multiply by xMant again:
        float term3_lo;
        float term3_hi = abacus::internal::multiply_exact(term2_hi, xMant1m,
&term3_lo);

        float term3_lo_lo;
        float term3_lo_hi = abacus::internal::multiply_exact(term2_lo_hi,
xMant1m, &term3_lo_lo);

        term3_lo_lo += term2_lo_hi*xMant1m;

        abacus::internal::add_exact(&term2_lo_hi, &term3_lo_lo);
        abacus::internal::add_exact(&term3_lo, &term2_lo_hi);
        abacus::internal::add_exact(&term3_hi, &term3_lo);

        //Add in xMant1m for the final answer
        abacus::internal::add_exact(&xMant1m, &term3_hi);
        abacus::internal::add_exact(&term3_hi, &term3_lo);
        abacus::internal::add_exact(&term3_hi, &term2_lo_hi);

        *out_remainder = term3_hi;

        return xMant1m;
}

inline float log2_extended_precision(float xMant, float * out_remainder){
        //Get the log first:
        float ln_lo;
        float ln_hi = log_extended_precision(xMant, &ln_lo);

        //an accurate 1/ln(2) (both needed)
        float recip_ln2_hi = 1.44269502162933349609375;
        float recip_ln2_lo =
1.9259629911266174681001892137426645954152985934135449406931109219181185079885526622893506345e-8;

        float log2_hi_lo;
        float log2_hi_hi = abacus::internal::multiply_exact(ln_hi, recip_ln2_hi,
&log2_hi_lo);


        *out_remainder = log2_hi_lo + (ln_lo*recip_ln2_hi + ln_hi*
recip_ln2_lo);

        return log2_hi_hi;
}
*/
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_LOG2_EXTENDED_PRECISION_H__
