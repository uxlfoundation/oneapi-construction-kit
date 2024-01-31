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
#include <abacus/internal/frexp_unsafe.h>
#include <abacus/internal/horner_polynomial.h>

namespace {
template <typename T>
T native_log(const T x) {
  // r = log(x), x = f * 2^n
  // r = log(f * 2^n)
  // r = log(f) + log(2^n)
  // r = log(f) + log2(2^n) / log2(e)
  // r = log(f) + n / log2(e)
  // r = f * log(f - 1) + n / log2(e)
  typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type n;
  const T f = abacus::internal::frexp_unsafe(x, &n);

  const abacus_float polynomial[3] = {1.00229863224844f, -.423525457684207f,
                                      .676185676420138f};

  // helper calculates log(f + 1) / f
  const T logf = abacus::internal::horner_polynomial(f - 1, polynomial);

  const T oneOverlog2e = 0.693147180559945309417232121458;
  return f * logf + abacus::detail::cast::convert<T>(n) * oneOverlog2e;
}
}  // namespace

abacus_float ABACUS_API __abacus_native_log(abacus_float x) {
  return native_log<>(x);
}
abacus_float2 ABACUS_API __abacus_native_log(abacus_float2 x) {
  return native_log<>(x);
}
abacus_float3 ABACUS_API __abacus_native_log(abacus_float3 x) {
  return native_log<>(x);
}
abacus_float4 ABACUS_API __abacus_native_log(abacus_float4 x) {
  return native_log<>(x);
}
abacus_float8 ABACUS_API __abacus_native_log(abacus_float8 x) {
  return native_log<>(x);
}
abacus_float16 ABACUS_API __abacus_native_log(abacus_float16 x) {
  return native_log<>(x);
}
