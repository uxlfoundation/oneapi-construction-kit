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
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T>
T face_forward(const T n, const T i, const T nref) {
  return (__abacus_dot(nref, i) < 0) ? n : -n;
}
}  // namespace

abacus_float ABACUS_API __abacus_face_forward(abacus_float n, abacus_float i,
                                              abacus_float nref) {
  return face_forward<>(n, i, nref);
}
abacus_float2 ABACUS_API __abacus_face_forward(abacus_float2 n, abacus_float2 i,
                                               abacus_float2 nref) {
  return face_forward<>(n, i, nref);
}
abacus_float3 ABACUS_API __abacus_face_forward(abacus_float3 n, abacus_float3 i,
                                               abacus_float3 nref) {
  return face_forward<>(n, i, nref);
}
abacus_float4 ABACUS_API __abacus_face_forward(abacus_float4 n, abacus_float4 i,
                                               abacus_float4 nref) {
  return face_forward<>(n, i, nref);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_face_forward(abacus_double n, abacus_double i,
                                               abacus_double nref) {
  return face_forward<>(n, i, nref);
}
abacus_double2 ABACUS_API __abacus_face_forward(abacus_double2 n,
                                                abacus_double2 i,
                                                abacus_double2 nref) {
  return face_forward<>(n, i, nref);
}
abacus_double3 ABACUS_API __abacus_face_forward(abacus_double3 n,
                                                abacus_double3 i,
                                                abacus_double3 nref) {
  return face_forward<>(n, i, nref);
}
abacus_double4 ABACUS_API __abacus_face_forward(abacus_double4 n,
                                                abacus_double4 i,
                                                abacus_double4 nref) {
  return face_forward<>(n, i, nref);
}
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
