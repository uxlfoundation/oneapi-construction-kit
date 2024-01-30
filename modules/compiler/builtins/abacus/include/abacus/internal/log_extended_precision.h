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

#ifndef __ABACUS_INTERNAL_LOG_EXTENDED_PRECISION_H__
#define __ABACUS_INTERNAL_LOG_EXTENDED_PRECISION_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>
#include <abacus/internal/add_exact.h>
#include <abacus/internal/horner_polynomial.h>
#include <abacus/internal/multiply_exact.h>
#include <abacus/internal/multiply_exact_unsafe.h>

// see maple worksheet for coefficient derivation
static ABACUS_CONSTANT abacus_double
    __codeplay_natural_log_extended_precision_coeffD[26] = {
        -0.25,
        0.2,
        -.1666666666666666670674284354921,
        .1428571428571428343862661055775,
        -.1249999999999997054342210013926,
        .1111111111111198755294141956370,
        -.1000000000000723456733225646952,
        0.9090909090784552487628727267937e-1,
        -0.8333333332453804940890798512256e-1,
        0.7692307701232649867470080522836e-1,
        -0.7142857204602006385171678689833e-1,
        0.6666666306651934386373766797132e-1,
        -0.6249997303511765130722153359299e-1,
        0.5882361192275476926509195129568e-1,
        -0.5555631385236985514044903763201e-1,
        0.5263067051745643056652761825633e-1,
        -0.4998619833293573931456855873584e-1,
        0.4761726375858645234004197646356e-1,
        -0.4561242352394073504725051180842e-1,
        0.4366424439531010825757412221774e-1,
        -0.4063720720136112411550557178031e-1,
        0.3769683278502438840520639968952e-1,
        -0.4107840930520312688576054431781e-1,
        0.4908099294159941209677222540895e-1,
        -0.4207569410330950970126703826312e-1,
        0.1633271041929616450610115728061e-1};

namespace abacus {
namespace internal {

// Accurate natural log
template <typename T>
inline T log_extended_precision(const T &xMant, T *out_remainder) {
  const T xMant1m = xMant - 1.0f;
  const T poly = abacus::internal::horner_polynomial(
      xMant1m, __codeplay_natural_log_extended_precision_coeffD);

  // We need xMant1m*(xMant1m*(xMant1m*(xMant1m*poly + 1/3) - 0.5) + 1)
  // accurately
  T first_term_const =
      0.333333333333333314829616256247390992939472198486328125;  // closest
                                                                 // thing to 1/3
                                                                 // in float
                                                                 // Causes
                                                                 // accuracy
                                                                 // problems
  const T error_in_1_over_3 =
      1.85037170770859423403938611348470052083333333333333333e-17;  // Error
                                                                    // between
                                                                    // 1/3 and
                                                                    // first_term_const

  T first_term_lo;
  T first_term_hi =
      abacus::internal::multiply_exact_unsafe(xMant1m, poly, &first_term_lo);

  abacus::internal::add_exact(&first_term_const, &first_term_hi);
  const T small_correction_term = xMant1m * error_in_1_over_3;

  first_term_hi += (first_term_lo + small_correction_term);

  first_term_lo = first_term_hi;
  first_term_hi = first_term_const;

  //--------------------------------------------------------------------------
  // next term
  T second_term_const = -0.5;
  T second_term_hi_lo;
  T second_term_hi_hi = abacus::internal::multiply_exact_unsafe(
      xMant1m, first_term_hi, &second_term_hi_lo);
  T second_term_lo_lo;
  T second_term_lo_hi = abacus::internal::multiply_exact_unsafe(
      xMant1m, first_term_lo, &second_term_lo_lo);

  second_term_hi_lo += second_term_lo_hi;  // Exact
  abacus::internal::add_exact(&second_term_const, &second_term_hi_hi);
  second_term_hi_hi += second_term_hi_lo;  // Exact

  //--------------------------------------------------------------------------
  // we multiply (second_term_const + second_term_hi_hi) by xMant1m and add 1.

  T third_term_const = 1.0;
  T third_term_hi_lo;
  T third_term_hi_hi = abacus::internal::multiply_exact_unsafe(
      xMant1m, second_term_const, &third_term_hi_lo);
  T third_term_lo_lo;
  T third_term_lo_hi = abacus::internal::multiply_exact_unsafe(
      xMant1m, second_term_hi_hi, &third_term_lo_lo);

  abacus::internal::add_exact(&third_term_hi_lo, &third_term_lo_hi);

  abacus::internal::add_exact(&third_term_const, &third_term_hi_hi);
  abacus::internal::add_exact(&third_term_hi_hi, &third_term_hi_lo);

  third_term_hi_lo = third_term_lo_hi;
  third_term_lo_hi = third_term_lo_lo;

  third_term_hi_lo += third_term_lo_hi;
  //--------------------------------------------------------------------------
  // multiply by xMant1m

  T final_term_hi_lo;
  const T final_term_hi_hi = abacus::internal::multiply_exact_unsafe(
      xMant1m, third_term_const, &final_term_hi_lo);
  const T final_term_lo_hi = xMant1m * third_term_hi_hi;

  // Add all this stuff together stuff
  final_term_hi_lo += final_term_lo_hi;

  *out_remainder = final_term_hi_lo;

  return final_term_hi_hi;
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_LOG_EXTENDED_PRECISION_H__
