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

#ifndef __ABACUS_INTERNAL_LOG2_EXTENDED_PRECISION_H__
#define __ABACUS_INTERNAL_LOG2_EXTENDED_PRECISION_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>
#include <abacus/internal/add_exact.h>
#include <abacus/internal/horner_polynomial.h>
#ifdef __CA_BUILTINS_HALF_SUPPORT
#include <abacus/internal/ldexp_unsafe.h>
#endif // __CA_BUILTINS_HALF_SUPPORT
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
#include <abacus/internal/log_extended_precision.h>
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
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
#endif // __CA_BUILTINS_HALF_SUPPORT

// Aprroximation of log2(x+1) between [sqrt(0.5)-1;2*sqrt(0.5)-1]
// See log2_extended_precision.sollya for derivation
// The first terms are separated out to avoid double-precision arithmetic.
static ABACUS_CONSTANT abacus_float __codeplay_log2_extended_precision_coeff[] =
    {0.33333301f,  -0.25000026f, 0.20002578f,   -0.16667923f, 0.14212508f,
     -0.12400908f, 0.11926104f,  -0.117190584f, 0.067263625f};
} // namespace

namespace abacus {
namespace internal {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct log2_extended_precision_helper;

template <typename T> struct log2_extended_precision_helper<T, abacus_float> {
  static T _(const T &xMant, T *out_remainder) {
    T xMAnt1m = xMant - 1.0f;

    T hi = xMAnt1m * abacus::internal::horner_polynomial(
                         xMAnt1m, __codeplay_log2_extended_precision_coeff);
    T lo = 0;

    auto AddF = [](T &hi, T &lo, T val) {
      abacus::internal::add_exact(&hi, &val);
      abacus::internal::add_exact(&lo, &val);
    };
    auto MulF = [](T &hi, T &lo, T val) {
      T mullo;
      hi = abacus::internal::multiply_exact(hi, val, &mullo);
      lo = __abacus_fma(val, lo, mullo);
    };
    auto MulD = [&AddF, &MulF](T &hi, T &lo, T hival, T loval) {
      T hihi = hi, hilo = lo, lohi = hi, lolo = lo;
      MulF(hihi, hilo, hival);
      MulF(lohi, lolo, loval);

      hi = lohi;
      lo = lolo;
      AddF(hi, lo, hilo);
      AddF(hi, lo, hihi);
    };

    AddF(hi, lo, -0.5f);
    AddF(hi, lo, 3.3101797e-09f);
    MulF(hi, lo, xMAnt1m);
    AddF(hi, lo, 1.f);
    AddF(hi, lo, 6.3439454e-10f);
    MulF(hi, lo, xMAnt1m);
    MulD(hi, lo, 1.442695f, 1.925963e-08f);

    *out_remainder = lo;
    return hi;
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T> struct log2_extended_precision_helper<T, abacus_double> {
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
#endif // __CA_BUILTINS_DOUBLE_SUPPORT

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

  abacus::internal::add_exact_unsafe(&c_term_hi, &poly_hi);
  // This adds in exactly, so no need for an add_exact
  c_term_lo = c_term_lo + poly_lo;

  abacus::internal::add_exact_unsafe(&c_term_lo, &poly_hi);
  abacus::internal::add_exact_unsafe(&c_term_hi, &c_term_lo);

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

  abacus::internal::add_exact_unsafe(&c_term_hi, &poly_hi);
  // This adds in exactly, so no need for an add_exact
  c_term_lo = c_term_lo + poly_lo;

  abacus::internal::add_exact_unsafe(&c_term_lo, &poly_hi);
  abacus::internal::add_exact_unsafe(&c_term_hi, &c_term_lo);

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

#endif // __CA_BUILTINS_HALF_SUPPORT
} // namespace internal
} // namespace abacus

#endif //__ABACUS_INTERNAL_LOG2_EXTENDED_PRECISION_H__
