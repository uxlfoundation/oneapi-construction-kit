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

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/internal/half_range_reduction.h>

namespace {
template <typename T>
T half_tan(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  SignedType octet;
  T xReduced = abacus::internal::half_range_reduction(x, &octet);

  T xSquared = xReduced * xReduced;

  T tan_numerator = ((T)1.157866227f - (T)0.07954878635f * xSquared) * xReduced;
  T tan_denominator = ((T)1.157857119f - ((T).4652878584f * xSquared));

  octet = octet & 0x3;

  T top = tan_numerator;
  T bottom = tan_denominator;

  top =
      __abacus_select(top, tan_denominator + tan_numerator, (octet & 0x1) != 0);
  bottom = __abacus_select(bottom, tan_denominator - tan_numerator,
                           (octet & 0x1) != 0);

  const SignedType octetGreaterOne = octet > 1;
  T ans = __abacus_select(top, -bottom, octetGreaterOne) /
          __abacus_select(bottom, top, octetGreaterOne);

  // Note this is not required by the spec but bruteforce expects a
  // value in the range [-1 .. 1] for out of range values.
  ans = __abacus_select(ans, 0.0f, __abacus_fabs(x) > 65536.0f);

  return __abacus_select(ABACUS_NAN, ans, __abacus_isfinite(x));
}
}  // namespace

abacus_float ABACUS_API __abacus_half_tan(abacus_float x) {
  return half_tan<>(x);
}
abacus_float2 ABACUS_API __abacus_half_tan(abacus_float2 x) {
  return half_tan<>(x);
}
abacus_float3 ABACUS_API __abacus_half_tan(abacus_float3 x) {
  return half_tan<>(x);
}
abacus_float4 ABACUS_API __abacus_half_tan(abacus_float4 x) {
  return half_tan<>(x);
}
abacus_float8 ABACUS_API __abacus_half_tan(abacus_float8 x) {
  return half_tan<>(x);
}
abacus_float16 ABACUS_API __abacus_half_tan(abacus_float16 x) {
  return half_tan<>(x);
}
