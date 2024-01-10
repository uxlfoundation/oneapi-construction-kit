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
template <typename T, typename TCoef>
inline T horner_polynomial(const T x, const TCoef *p_coef, size_t N) {
  T coef_sum = T(p_coef[N - 1]);

  for (size_t n = N - 1; n > 0; n--) {
    if constexpr (sizeof(typename TypeTraits<T>::ElementType) == 2) {  // NOLINT
      // For half, we need the precision of FMA
      coef_sum = __abacus_fma(coef_sum, x, T(p_coef[n - 1]));
    } else {  // NOLINT
      coef_sum = T(p_coef[n - 1]) + x * coef_sum;
    }
  }

  return coef_sum;
}

template <typename T, size_t N, typename TCoef>
inline T horner_polynomial(const T x, const TCoef (&coef)[N]) {
  return horner_polynomial(x, coef, N);
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_HORNER_POLYNOMIAL_H__
