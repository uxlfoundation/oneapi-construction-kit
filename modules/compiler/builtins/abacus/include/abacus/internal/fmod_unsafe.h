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

#ifndef __ABACUS_INTERNAL_EXP_UNSAFE_H__
#define __ABACUS_INTERNAL_EXP_UNSAFE_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/float_construct.h>
#include <abacus/internal/float_deconstruct.h>

namespace abacus {
namespace internal {

template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct loop_info;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct loop_info<T, abacus_half> {
  // Max difference between unbiased exponents is 32, we can avoid doing any
  // iterations of the loop and just perform the single case after it.
  static const abacus_uint iterations = 0;
  static const abacus_int shift = 32;
};
#endif

template <typename T>
struct loop_info<T, abacus_float> {
  // Max difference between unbiased exponents is 256, so we at most need 6.4
  // iterations 6 within the loop and the one case just after it
  static const abacus_uint iterations = 6;
  static const abacus_int shift = 40;
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct loop_info<T, abacus_double> {
  // Max difference between unbiased exponents is 2047, so we at most need 186.1
  // iterations 186 within the loop and the one case just after it
  static const abacus_uint iterations = 186;
  static const abacus_int shift = 11;
};
#endif

template <typename T, bool USE_QUOTIENT,
          unsigned N = TypeTraits<T>::num_elements>
struct fmod_helper {
  typedef
      typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type IntType;
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;
  typedef typename MakeType<abacus_long, TypeTraits<T>::num_elements>::type
      SLongType;
  typedef typename TypeTraits<SLongType>::UnsignedType ULongType;

  static T _(const T x, const T m, IntType *out_quotient) {
    const T xAbs = __abacus_fabs(x);
    const T mAbs = __abacus_fabs(m);

    UnsignedType xExp, mExp;

    // Get ints such that the binary of the ints is the same as the mantissa of
    // the floats. Checks for denorms.
    const ULongType xMant = abacus::detail::cast::convert<ULongType>(
        abacus::internal::float_deconstruct(xAbs, &xExp));
    const ULongType mMant = abacus::detail::cast::convert<ULongType>(
        abacus::internal::float_deconstruct(mAbs, &mExp));

    // make mMant 1 if it was 0
    const ULongType mMantFudged = __abacus_select(mMant, 1, mMant == 0);

    // Get the exponent difference between them: Note xExp >= mExp
    const UnsignedType xExpNew = __abacus_select(xExp - mExp, 0, xAbs < mAbs);

    // The sum is now finding: xMant * 2^xExpNew mod(mMant)
    ULongType quotient = xMant / mMantFudged;
    ULongType ansMant = xMant - (quotient * mMantFudged);

    // i is initialized to the difference between unbiased exponents
    ULongType i = abacus::detail::cast::convert<ULongType>(xExpNew);
    for (abacus_uint k = 0; k < loop_info<T>::iterations; k++) {
      const SLongType cond = i >= loop_info<T>::shift;
      const ULongType temp = ansMant << loop_info<T>::shift;
      const ULongType r = temp / mMantFudged;
      ansMant = __abacus_select(ansMant, temp - r * mMantFudged, cond);
      quotient = __abacus_select(quotient,
                                 (quotient << loop_info<T>::shift) + r, cond);
      i = __abacus_select(i, i - loop_info<T>::shift, cond);
    }

    ansMant = ansMant << i;
    const ULongType r = ansMant / mMantFudged;
    ansMant -= r * mMantFudged;
    quotient = ((quotient << i) + r);

    // ansMant has the mantissa of fmod in it now.
    T result = abacus::internal::float_construct(
        abacus::detail::cast::convert<UnsignedType>(ansMant), mExp);

    result = __abacus_select(result, xAbs, xAbs < mAbs);

    if (USE_QUOTIENT) {
      const IntType cond = abacus::detail::cast::convert<IntType>(xAbs < mAbs);
      // 0x7F since OpenCL spec mandates 7-bits of precision in quotient
      const IntType quot =
          abacus::detail::cast::convert<IntType>(quotient) & 0x7f;
      *out_quotient = __abacus_select(quot, (IntType)0, cond);
    }

    return result;
  }
};

template <typename T, bool USE_QUOTIENT>
struct fmod_helper<T, USE_QUOTIENT, 1u> {
  static T _(const T x, const T m, abacus_int *out_quotient) {
    typedef typename TypeTraits<T>::UnsignedType UnsignedType;
    typedef typename TypeTraits<T>::SignedType SignedType;

    const T xAbs = __abacus_fabs(x);
    const T mAbs = __abacus_fabs(m);

    if (xAbs < mAbs) {
      if (USE_QUOTIENT) {
        *out_quotient = 0;
      }
      return xAbs;
    }

    UnsignedType xExp, mExp;

    // Get ints such that the binary of the ints is the same as the mantissa of
    // the floats. Checks for denorms.
    const UnsignedType xMant = abacus::internal::float_deconstruct(xAbs, &xExp);
    const UnsignedType mMant = abacus::internal::float_deconstruct(mAbs, &mExp);

    // make mMant 1 if it was 0
    const UnsignedType mMantFudged = mMant == 0 ? 1 : mMant;

    // Get the exponent difference between them: Note xExp >= mExp
    const SignedType xExpNew = (SignedType)xExp - (SignedType)mExp;

    // The sum is now finding: xMant * 2^xExpNew mod(mMant)
    abacus_ulong quotient = xMant / mMantFudged;
    abacus_ulong ansMant = xMant - (quotient * mMant);

    SignedType i = xExpNew;
    for (; i >= loop_info<T>::shift; i -= loop_info<T>::shift) {
      ansMant = ansMant << loop_info<T>::shift;
      const abacus_ulong r = ansMant / mMantFudged;
      ansMant -= r * mMant;
      quotient = ((quotient << loop_info<T>::shift) + r);
    }

    ansMant = ansMant << i;
    const abacus_ulong r = ansMant / mMantFudged;
    ansMant -= r * mMant;
    quotient = ((quotient << i) + r);

    T result = abacus::internal::float_construct((UnsignedType)ansMant, mExp);

    if (USE_QUOTIENT) {
      // 0x7F since OpenCL spec mandates 7-bits of precision in quotient
      *out_quotient = ((abacus_int)quotient & 0x7f);
    }

    return result;
  }
};

template <typename T>
inline T fmod_unsafe(const T x, const T m) {
  return fmod_helper<T, false>::_(x, m, 0);
}

template <typename T>
inline T fmod_unsafe(
    const T x, const T m,
    typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type
        *quotient) {
  return fmod_helper<T, true>::_(x, m, quotient);
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_EXP_UNSAFE_H__
