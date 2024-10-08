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
#include <abacus/abacus_extra.h>
#include <abacus/abacus_geometric.h>

namespace {
template <typename T>
T reflect(const T i, const T n) {
  return i - (n * __abacus_dot(i, n) * 2);
}
}  // namespace

abacus_float ABACUS_API __abacus_reflect(abacus_float i, abacus_float n) {
  return reflect<>(i, n);
}
abacus_float2 ABACUS_API __abacus_reflect(abacus_float2 i, abacus_float2 n) {
  return reflect<>(i, n);
}
abacus_float3 ABACUS_API __abacus_reflect(abacus_float3 i, abacus_float3 n) {
  return reflect<>(i, n);
}
abacus_float4 ABACUS_API __abacus_reflect(abacus_float4 i, abacus_float4 n) {
  return reflect<>(i, n);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_reflect(abacus_double i, abacus_double n) {
  return reflect<>(i, n);
}
abacus_double2 ABACUS_API __abacus_reflect(abacus_double2 i, abacus_double2 n) {
  return reflect<>(i, n);
}
abacus_double3 ABACUS_API __abacus_reflect(abacus_double3 i, abacus_double3 n) {
  return reflect<>(i, n);
}
abacus_double4 ABACUS_API __abacus_reflect(abacus_double4 i, abacus_double4 n) {
  return reflect<>(i, n);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
