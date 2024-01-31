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

#ifndef __ABACUS_INTERNAL_HALF_SINCOS_APPROX_H__
#define __ABACUS_INTERNAL_HALF_SINCOS_APPROX_H__

#include <abacus/abacus_config.h>
#include <abacus/internal/horner_polynomial.h>

static ABACUS_CONSTANT abacus_float _half_sincos_coefc[4] = {
    1.0f, -0.4999988475f, 0.4165577706e-1f, -0.1359185355e-2f};
static ABACUS_CONSTANT abacus_float _half_sincos_coefs[4] = {
    0.9999999969f, -0.1666665022f, 0.008332016456f, -0.0001950182203f};

namespace abacus {
namespace internal {
template <typename T>
inline T half_sincos_approx(const T &x, T *cosVal) {
  // TODO since sin and cos both call this now it might be worth looking at
  // better ways to make it efficient, as I wrote it quite quickly

  // minimax polynomials from 0 -> PI/4
  const T xx = x * x;
  *cosVal = abacus::internal::horner_polynomial(xx, _half_sincos_coefc);

  return x * abacus::internal::horner_polynomial(xx, _half_sincos_coefs);
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_HALF_SINCOS_APPROX_H__
