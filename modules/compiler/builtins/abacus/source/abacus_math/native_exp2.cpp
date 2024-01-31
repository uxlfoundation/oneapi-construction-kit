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
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/floor_unsafe.h>
#include <abacus/internal/horner_polynomial.h>
#include <abacus/internal/ldexp_unsafe.h>

namespace {
template <typename T>
T native_exp2(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  // r = 2^x
  // r = 2^(k + f), k = floor(x), f = x - floor(f)
  // r = 2^k * 2^f
  const SignedType k = abacus::internal::floor_unsafe(x);
  const T f = x - abacus::detail::cast::convert<T>(k);

  const abacus_float polynomial[3] = {1.00172475857779f, .657636286949233f,
                                      .337189437317397f};

  const T twoToTheF = abacus::internal::horner_polynomial(f, polynomial);
  return abacus::internal::ldexp_unsafe(twoToTheF, k);
}
}  // namespace

abacus_float ABACUS_API __abacus_native_exp2(abacus_float x) {
  return native_exp2<>(x);
}
abacus_float2 ABACUS_API __abacus_native_exp2(abacus_float2 x) {
  return native_exp2<>(x);
}
abacus_float3 ABACUS_API __abacus_native_exp2(abacus_float3 x) {
  return native_exp2<>(x);
}
abacus_float4 ABACUS_API __abacus_native_exp2(abacus_float4 x) {
  return native_exp2<>(x);
}
abacus_float8 ABACUS_API __abacus_native_exp2(abacus_float8 x) {
  return native_exp2<>(x);
}
abacus_float16 ABACUS_API __abacus_native_exp2(abacus_float16 x) {
  return native_exp2<>(x);
}
