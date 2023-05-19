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

#ifndef __ABACUS_CHECK_SURROUNDING_VALUES_H__
#define __ABACUS_CHECK_SURROUNDING_VALUES_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/multiply_exact_unsafe.h>

namespace abacus {
namespace internal {
// Checks the 3 values surrounding sqrt_estimate and returns the one
// mathematically closest to sqrt(input)
template <typename T>
inline T check_surrounding_values(const T &input, const T &sqrt_estimate) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;
  // We now need to check if the values on either side of sqrt_value are better
  // approximations of sqrt(input) seems to be up to one bit off
  const UnsignedType sqrtAs =
      abacus::detail::cast::as<UnsignedType>(sqrt_estimate);
  const T sqrt_value_lo = abacus::detail::cast::as<T>(sqrtAs - 1);
  const T sqrt_value_hi = abacus::detail::cast::as<T>(sqrtAs + 1);

  // Calculate  0.5*(sqrt_estimate^2 - input) very accurately for each of the 3
  // possibilities:
  T can1sq_lo;
  const T can1sq_hi = abacus::internal::multiply_exact_unsafe(
      sqrt_value_lo, sqrt_value_lo, &can1sq_lo);
  const T term1 = ((can1sq_hi - input) + can1sq_lo) * 0.5;

  T can2sq_lo;
  const T can2sq_hi = abacus::internal::multiply_exact_unsafe(
      sqrt_estimate, sqrt_estimate, &can2sq_lo);
  const T term2 = ((can2sq_hi - input) + can2sq_lo) * 0.5;

  T can3sq_lo;
  const T can3sq_hi = abacus::internal::multiply_exact_unsafe(
      sqrt_value_hi, sqrt_value_hi, &can3sq_lo);
  const T term3 = ((can3sq_hi - input) + can3sq_lo) * 0.5;

  T result = sqrt_estimate;

  const SignedType c1 = term2 + term3 < input - sqrt_value_hi * sqrt_estimate;
  result = __abacus_select(result, sqrt_value_hi, c1);

  const SignedType c2 = term1 + term2 >= input - sqrt_value_lo * sqrt_estimate;
  result = __abacus_select(result, sqrt_value_lo, c2);

  return result;
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_CHECK_SURROUNDING_VALUES_H__
