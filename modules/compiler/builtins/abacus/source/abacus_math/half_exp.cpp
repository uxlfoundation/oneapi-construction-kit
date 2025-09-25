// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
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

template <typename T>
static T half_exp(T x) {
  return __abacus_half_exp2(x * ABACUS_LOG2E_F);
}

abacus_float ABACUS_API __abacus_half_exp(abacus_float x) {
  return half_exp<>(x);
}
abacus_float2 ABACUS_API __abacus_half_exp(abacus_float2 x) {
  return half_exp<>(x);
}
abacus_float3 ABACUS_API __abacus_half_exp(abacus_float3 x) {
  return half_exp<>(x);
}
abacus_float4 ABACUS_API __abacus_half_exp(abacus_float4 x) {
  return half_exp<>(x);
}
abacus_float8 ABACUS_API __abacus_half_exp(abacus_float8 x) {
  return half_exp<>(x);
}
abacus_float16 ABACUS_API __abacus_half_exp(abacus_float16 x) {
  return half_exp<>(x);
}
