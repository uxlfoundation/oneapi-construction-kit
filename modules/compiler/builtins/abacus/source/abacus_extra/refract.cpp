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
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T>
T refract(const T i, const T n, const typename TypeTraits<T>::ElementType eta) {
  typedef typename TypeTraits<T>::ElementType ElementType;

  const ElementType intermediate = __abacus_dot(n, i);
  const ElementType k = 1 - (eta * eta * (1 - intermediate * intermediate));

  const T result = (i * eta) - (n * (eta * intermediate + __abacus_sqrt(k)));

  return (k < 0) ? (T)0 : result;
}
}  // namespace

abacus_float ABACUS_API __abacus_refract(abacus_float i, abacus_float n,
                                         abacus_float eta) {
  return refract<>(i, n, eta);
}
abacus_float2 ABACUS_API __abacus_refract(abacus_float2 i, abacus_float2 n,
                                          abacus_float eta) {
  return refract<>(i, n, eta);
}
abacus_float3 ABACUS_API __abacus_refract(abacus_float3 i, abacus_float3 n,
                                          abacus_float eta) {
  return refract<>(i, n, eta);
}
abacus_float4 ABACUS_API __abacus_refract(abacus_float4 i, abacus_float4 n,
                                          abacus_float eta) {
  return refract<>(i, n, eta);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_refract(abacus_double i, abacus_double n,
                                          abacus_float eta) {
  return refract<>(i, n, eta);
}
abacus_double2 ABACUS_API __abacus_refract(abacus_double2 i, abacus_double2 n,
                                           abacus_float eta) {
  return refract<>(i, n, eta);
}
abacus_double3 ABACUS_API __abacus_refract(abacus_double3 i, abacus_double3 n,
                                           abacus_float eta) {
  return refract<>(i, n, eta);
}
abacus_double4 ABACUS_API __abacus_refract(abacus_double4 i, abacus_double4 n,
                                           abacus_float eta) {
  return refract<>(i, n, eta);
}

abacus_double ABACUS_API __abacus_refract(abacus_double i, abacus_double n,
                                          abacus_double eta) {
  return refract<>(i, n, eta);
}
abacus_double2 ABACUS_API __abacus_refract(abacus_double2 i, abacus_double2 n,
                                           abacus_double eta) {
  return refract<>(i, n, eta);
}
abacus_double3 ABACUS_API __abacus_refract(abacus_double3 i, abacus_double3 n,
                                           abacus_double eta) {
  return refract<>(i, n, eta);
}
abacus_double4 ABACUS_API __abacus_refract(abacus_double4 i, abacus_double4 n,
                                           abacus_double eta) {
  return refract<>(i, n, eta);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
