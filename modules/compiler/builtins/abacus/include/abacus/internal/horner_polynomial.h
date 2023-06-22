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

#ifndef __ABACUS_INTERNAL_HORNER_POLYNOMIAL_H__
#define __ABACUS_INTERNAL_HORNER_POLYNOMIAL_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_type_traits.h>

namespace abacus {
namespace internal {
#ifdef __CA_BUILTINS_HALF_SUPPORT
inline abacus_half horner_polynomial(const abacus_half x,
                                     ABACUS_CONSTANT abacus_half *p_coef,
                                     int N) {
  abacus_half coef_sum = p_coef[N - 1];

  for (int n = N - 1; n > 0; n--) {
    coef_sum = p_coef[n - 1] + x * coef_sum;
  }

  return coef_sum;
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

inline abacus_float horner_polynomial(const abacus_float x,
                                      ABACUS_CONSTANT abacus_float *p_coef,
                                      int N) {
  abacus_float coef_sum = p_coef[N - 1];

  for (int n = N - 1; n > 0; n--) {
    coef_sum = p_coef[n - 1] + x * coef_sum;
  }

  return coef_sum;
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
inline abacus_double horner_polynomial(const abacus_double x,
                                       ABACUS_CONSTANT abacus_double *p_coef,
                                       int N) {
  abacus_double coef_sum = p_coef[N - 1];

  for (int n = N - 1; n > 0; n--) {
    coef_sum = p_coef[n - 1] + x * coef_sum;
  }

  return coef_sum;
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T, unsigned int N>
inline T horner_polynomial(
    const T &x, const typename TypeTraits<T>::ElementType *coefficients) {
  T sum = coefficients[N - 1];

  for (unsigned int n = N - 1; n > 0; n--) {
    sum = ((T)coefficients[n - 1]) + x * sum;
  }

  return sum;
}

#ifdef __OPENCL_VERSION__
template <typename T, unsigned int N>
inline T horner_polynomial(const T &x, ABACUS_CONSTANT
                           typename TypeTraits<T>::ElementType *coefficients) {
  T sum = coefficients[N - 1];

  for (unsigned int n = N - 1; n > 0; n--) {
    sum = ((T)coefficients[n - 1]) + x * sum;
  }

  return sum;
}
#endif
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_HORNER_POLYNOMIAL_H__
