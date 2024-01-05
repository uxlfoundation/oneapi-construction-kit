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
#include <abacus/internal/frexp_unsafe.h>
#include <abacus/internal/horner_polynomial.h>

namespace {
template <typename T>
T native_log2(const T x) {
  // r = log2(x), x = f * 2^n
  // r = log2(f * 2^n)
  // r = log2(f) + log2(2^n)
  // r = log2(f) + n
  typedef typename TypeTraits<T>::SignedType SignedType;
  typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type n;
  const T f = abacus::internal::frexp_unsafe(x, &n);

  // Polynomial aproximates log2(x) within a range of 0.5<x<1, the range of
  // possible fractionals for positive floats. The coefficients here were
  // calculated via polynomial regression on an evenly distributed range of 20
  // log2(x) values for 0.5 < x < 1. The actual regression was done with
  // mycurvefit.com, a simple online tool.
  const abacus_float polynomial[6] = {-3.810813f, 10.26252f,  -14.43957f,
                                      13.39757f,  -6.918097f, 1.508444f};
  const T log2f = abacus::internal::horner_polynomial(f, polynomial);

  // Make the effort to return NaN and inf in the appropriate cases (x<0 and
  // x==0 repectively).
  const SignedType condNan = x < 0;
  const SignedType condInf = x == 0;
  T r = __abacus_select(log2f + abacus::detail::cast::convert<T>(n), ABACUS_NAN,
                        condNan);
  r = __abacus_select(r, ABACUS_INFINITY, condInf);
  return r;
}
}  // namespace

abacus_float ABACUS_API __abacus_native_log2(abacus_float x) {
  return native_log2<>(x);
}
abacus_float2 ABACUS_API __abacus_native_log2(abacus_float2 x) {
  return native_log2<>(x);
}
abacus_float3 ABACUS_API __abacus_native_log2(abacus_float3 x) {
  return native_log2<>(x);
}
abacus_float4 ABACUS_API __abacus_native_log2(abacus_float4 x) {
  return native_log2<>(x);
}
abacus_float8 ABACUS_API __abacus_native_log2(abacus_float8 x) {
  return native_log2<>(x);
}
abacus_float16 ABACUS_API __abacus_native_log2(abacus_float16 x) {
  return native_log2<>(x);
}
