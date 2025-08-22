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

namespace {
template <typename T> T native_powr(const T x, const T y) {
  // r = x ^ y
  // r = 2 ^ (log2(x ^ y))
  // r = 2 ^ (y * log2(x))
  return __abacus_native_exp2(y * __abacus_native_log2(x));
}
} // namespace

abacus_float ABACUS_API __abacus_native_powr(abacus_float x, abacus_float y) {
  return native_powr<>(x, y);
}
abacus_float2 ABACUS_API __abacus_native_powr(abacus_float2 x,
                                              abacus_float2 y) {
  return native_powr<>(x, y);
}
abacus_float3 ABACUS_API __abacus_native_powr(abacus_float3 x,
                                              abacus_float3 y) {
  return native_powr<>(x, y);
}
abacus_float4 ABACUS_API __abacus_native_powr(abacus_float4 x,
                                              abacus_float4 y) {
  return native_powr<>(x, y);
}
abacus_float8 ABACUS_API __abacus_native_powr(abacus_float8 x,
                                              abacus_float8 y) {
  return native_powr<>(x, y);
}
abacus_float16 ABACUS_API __abacus_native_powr(abacus_float16 x,
                                               abacus_float16 y) {
  return native_powr<>(x, y);
}
