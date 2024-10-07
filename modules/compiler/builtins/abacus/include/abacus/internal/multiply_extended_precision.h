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

#ifndef __ABACUS_INTERNAL_MULTIPLY_EXTENDED_PRECISION_H__
#define __ABACUS_INTERNAL_MULTIPLY_EXTENDED_PRECISION_H__

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/internal/add_exact.h>
#include <abacus/internal/multiply_exact.h>

namespace abacus {
namespace internal {
template <typename T>
inline T multiply_extended_precision(
    T input_hi, T input_lo, T xExp_abacus_float,
    typename TypeTraits<T>::SignedType n,
    typename TypeTraits<T>::SignedType *floor_val) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  // this returns the mantissa and the floor value of n*(xExp + input_hi +
  // input_lo)
  const SignedType n_hi = n & 0xFFFF0000;
  const SignedType n_lo = n & 0x0000FFFF;

  T high_sum_lo;
  const T high_sum_hi =
      abacus::internal::add_exact<>(xExp_abacus_float, input_hi, &high_sum_lo);

  high_sum_lo += input_lo;

  T term1_lo;
  const T term1_hi = abacus::internal::multiply_exact<>(
      abacus::detail::cast::convert<T>(n_hi), high_sum_hi, &term1_lo);
  T term2_lo;
  const T term2_hi = abacus::internal::multiply_exact<>(
      abacus::detail::cast::convert<T>(n_lo), high_sum_hi, &term2_lo);

  term1_lo = term1_lo + term2_hi;

  T more_sum_lo;
  const T more_sum_hi =
      abacus::internal::add_exact<>(term1_hi, term1_lo, &more_sum_lo);

  const T half_sum = (more_sum_lo + term2_lo) +
                     (abacus::detail::cast::convert<T>(n) * high_sum_lo);

  T final_sum_lo;
  const T final_sum_hi =
      abacus::internal::add_exact<>(more_sum_hi, half_sum, &final_sum_lo);

  const T total_floor = __abacus_floor(final_sum_hi);

  *floor_val = abacus::detail::cast::convert<SignedType>(total_floor);

  return (final_sum_hi - total_floor) + final_sum_lo;
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_MULTIPLY_EXTENDED_PRECISION_H__
