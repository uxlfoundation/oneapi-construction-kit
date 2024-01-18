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

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/floor_unsafe.h>
#include <abacus/internal/horner_polynomial.h>
#include <abacus/internal/ldexp_unsafe.h>

static ABACUS_CONSTANT abacus_float __codeplay_half_exp2_coeff[4] = {
    .99993f, .69586f, .22604f, 0.78022e-1f};

namespace {
template <typename T>
T half_exp2(T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  const SignedType xGreater = __abacus_isgreaterequal(x, 128.0f);
  const SignedType xLess = __abacus_isless(x, -136.0f);
  if (__abacus_all(xGreater)) {
    return (T)ABACUS_INFINITY;
  } else if (__abacus_all(xLess)) {
    return (T)0.0f;
  }

  // r = 2^x
  // r = 2^(i + f)
  // r = 2^i * 2^f
  // r = ldexp(2^f, i)
  const SignedType xFloor = abacus::internal::floor_unsafe(x);
  T xMant = x - abacus::detail::cast::convert<T>(xFloor);

  T exp2_xMant =
      abacus::internal::horner_polynomial(xMant, __codeplay_half_exp2_coeff);

  T result = abacus::internal::ldexp_unsafe(exp2_xMant, xFloor);

  result = __abacus_select(result, (T)0.0f, xLess);
  return __abacus_select(result, (T)ABACUS_INFINITY, xGreater);
}
}  // namespace

abacus_float ABACUS_API __abacus_half_exp2(abacus_float x) {
  return half_exp2<>(x);
}
abacus_float2 ABACUS_API __abacus_half_exp2(abacus_float2 x) {
  return half_exp2<>(x);
}
abacus_float3 ABACUS_API __abacus_half_exp2(abacus_float3 x) {
  return half_exp2<>(x);
}
abacus_float4 ABACUS_API __abacus_half_exp2(abacus_float4 x) {
  return half_exp2<>(x);
}
abacus_float8 ABACUS_API __abacus_half_exp2(abacus_float8 x) {
  return half_exp2<>(x);
}
abacus_float16 ABACUS_API __abacus_half_exp2(abacus_float16 x) {
  return half_exp2<>(x);
}
