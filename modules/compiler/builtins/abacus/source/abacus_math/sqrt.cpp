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

namespace abacus {
namespace detail {
template <typename T>
void inplace_sqrt(T &);
}  // namespace detail

namespace internal {
template <typename T>
T sqrt(T x) {
  abacus::detail::inplace_sqrt(x);
  return x;
}
}  // namespace internal
}  // namespace abacus

#ifdef __CA_BUILTINS_HALF_SUPPORT
template abacus_half abacus::internal::sqrt(abacus_half);
template abacus_half2 abacus::internal::sqrt(abacus_half2);
template abacus_half3 abacus::internal::sqrt(abacus_half3);
template abacus_half4 abacus::internal::sqrt(abacus_half4);
template abacus_half8 abacus::internal::sqrt(abacus_half8);
template abacus_half16 abacus::internal::sqrt(abacus_half16);

abacus_half ABACUS_API __abacus_sqrt(abacus_half x) {
  return abacus::internal::sqrt<>(x);
}
abacus_half2 ABACUS_API __abacus_sqrt(abacus_half2 x) {
  return abacus::internal::sqrt<>(x);
}
abacus_half3 ABACUS_API __abacus_sqrt(abacus_half3 x) {
  return abacus::internal::sqrt<>(x);
}
abacus_half4 ABACUS_API __abacus_sqrt(abacus_half4 x) {
  return abacus::internal::sqrt<>(x);
}
abacus_half8 ABACUS_API __abacus_sqrt(abacus_half8 x) {
  return abacus::internal::sqrt<>(x);
}
abacus_half16 ABACUS_API __abacus_sqrt(abacus_half16 x) {
  return abacus::internal::sqrt<>(x);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

template abacus_float abacus::internal::sqrt(abacus_float);
template abacus_float2 abacus::internal::sqrt(abacus_float2);
template abacus_float3 abacus::internal::sqrt(abacus_float3);
template abacus_float4 abacus::internal::sqrt(abacus_float4);
template abacus_float8 abacus::internal::sqrt(abacus_float8);
template abacus_float16 abacus::internal::sqrt(abacus_float16);

abacus_float ABACUS_API __abacus_sqrt(abacus_float x) {
  return abacus::internal::sqrt<>(x);
}
abacus_float2 ABACUS_API __abacus_sqrt(abacus_float2 x) {
  return abacus::internal::sqrt<>(x);
}
abacus_float3 ABACUS_API __abacus_sqrt(abacus_float3 x) {
  return abacus::internal::sqrt<>(x);
}
abacus_float4 ABACUS_API __abacus_sqrt(abacus_float4 x) {
  return abacus::internal::sqrt<>(x);
}
abacus_float8 ABACUS_API __abacus_sqrt(abacus_float8 x) {
  return abacus::internal::sqrt<>(x);
}
abacus_float16 ABACUS_API __abacus_sqrt(abacus_float16 x) {
  return abacus::internal::sqrt<>(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template abacus_double abacus::internal::sqrt(abacus_double);
template abacus_double2 abacus::internal::sqrt(abacus_double2);
template abacus_double3 abacus::internal::sqrt(abacus_double3);
template abacus_double4 abacus::internal::sqrt(abacus_double4);
template abacus_double8 abacus::internal::sqrt(abacus_double8);
template abacus_double16 abacus::internal::sqrt(abacus_double16);

abacus_double ABACUS_API __abacus_sqrt(abacus_double x) {
  return abacus::internal::sqrt<>(x);
}
abacus_double2 ABACUS_API __abacus_sqrt(abacus_double2 x) {
  return abacus::internal::sqrt<>(x);
}
abacus_double3 ABACUS_API __abacus_sqrt(abacus_double3 x) {
  return abacus::internal::sqrt<>(x);
}
abacus_double4 ABACUS_API __abacus_sqrt(abacus_double4 x) {
  return abacus::internal::sqrt<>(x);
}
abacus_double8 ABACUS_API __abacus_sqrt(abacus_double8 x) {
  return abacus::internal::sqrt<>(x);
}
abacus_double16 ABACUS_API __abacus_sqrt(abacus_double16 x) {
  return abacus::internal::sqrt<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
