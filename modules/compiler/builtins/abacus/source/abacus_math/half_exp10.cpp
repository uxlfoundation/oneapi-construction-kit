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
#include <abacus/abacus_math.h>

namespace {
template <typename T>
T half_exp10(T x) {
  const T log_e_base10 = (T)3.32192809489f;
  return __abacus_half_exp2(log_e_base10 * x);
}
}  // namespace

abacus_float ABACUS_API __abacus_half_exp10(abacus_float x) {
  return half_exp10<>(x);
}
abacus_float2 ABACUS_API __abacus_half_exp10(abacus_float2 x) {
  return half_exp10<>(x);
}
abacus_float3 ABACUS_API __abacus_half_exp10(abacus_float3 x) {
  return half_exp10<>(x);
}
abacus_float4 ABACUS_API __abacus_half_exp10(abacus_float4 x) {
  return half_exp10<>(x);
}
abacus_float8 ABACUS_API __abacus_half_exp10(abacus_float8 x) {
  return half_exp10<>(x);
}
abacus_float16 ABACUS_API __abacus_half_exp10(abacus_float16 x) {
  return half_exp10<>(x);
}
