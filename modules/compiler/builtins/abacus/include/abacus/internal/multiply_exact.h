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

#ifndef __ABACUS_INTERNAL_MULTIPLY_EXACT_H__
#define __ABACUS_INTERNAL_MULTIPLY_EXACT_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_type_traits.h>

// Perform exact multiplication of two floating point values.
//
// After hi = multiply_exact(x, y, &lo), unless overflow occurred, hi = x * y
// according to floating point rules, and hi + lo = x * y according to
// mathematical rules.
//
// Dekker, T.J. A floating-point technique for extending the available
// precision. Numer. Math. 18, 224–242 (1971).
// https://doi.org/10.1007/BF01397083

namespace {
template <typename E>
struct multiply_exact_helper {
  template <typename T>
  static void split(const T &x, T *x_hi, T *x_lo) {
    typedef typename TypeTraits<E>::SignedType SignedType;
    typedef typename TypeTraits<E>::UnsignedType UnsignedType;

    UnsignedType C = 1;
    // shift is the number of mantissa bits plus 1 for the implicit bit, then
    // divided by two as we're splitting `x` into two parts, rounded up as
    // described in section 6.3 with reference to 5.7.
    const SignedType shift = (FPShape<E>::Mantissa() / 2) + 1;
    C = (C << shift) + 1;
    const T gamma = x * abacus::detail::cast::convert<E>(C);
    const T delta = x - gamma;
    *x_hi = gamma + delta;
    *x_lo = x - *x_hi;
  }
};

template <>
struct multiply_exact_helper<abacus_float> {
  template <typename T>
  static void split(const T &x, T *x_hi, T *x_lo) {
    typedef typename TypeTraits<T>::UnsignedType UnsignedType;
    // Implementing split using this bitmask method means we don't need to
    // scale large inputs before calling multiply_exact() to prevent the final
    // remainder going to NaN, which occurs for the half and double split().
    // This bitmask covers the top half of the floating point number and
    // exactly half the mantissa bits (including the hidden bit). This ensures
    // that in the mantissas of the two resulting numbers (x_hi and x_lo) at
    // least half of the bits are 0 (in the lower half). Which in turn makes all
    // the ensuing multiplications exact.
    // This trick doesn't work in this form for 16 bit floats alas, as they have
    // an odd number of mantissa bits (including the hidden bit)
    *x_hi = abacus::detail::cast::as<T>(
        abacus::detail::cast::as<UnsignedType>(x) & 0xFFFFF000u);
    *x_lo = x - *x_hi;
  }
};
}  // namespace

namespace abacus {
namespace internal {
template <typename T>
inline T multiply_exact(const T x, const T y, T *out_remainder) {
  // TODO
  //  If fma is available on hardware this is probably faster
  //  float multiply = x*y;
  //  *remainder = fma(x,y, - multiply);
  typedef typename TypeTraits<T>::ElementType ElementType;

  T x_hi;
  T x_lo;

  multiply_exact_helper<ElementType>::split(x, &x_hi, &x_lo);

  // same with y
  T y_hi;
  T y_lo;

  multiply_exact_helper<ElementType>::split(y, &y_hi, &y_lo);

  const T r1 = x * y;
  const T t1 = -r1 + (x_hi * y_hi);
  const T t2 = t1 + (x_hi * y_lo);
  const T t3 = t2 + (x_lo * y_hi);

  *out_remainder = t3 + x_lo * y_lo;
  return r1;
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_MULTIPLY_EXACT_H__
