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
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T>
inline T nan_helper() {
  // Return NaN with all exponent bits and least significant bit set
  const typename TypeTraits<T>::UnsignedType nanVal =
      FPShape<T>::ExponentMask() | 0x1;
  return abacus::detail::cast::as<T>(nanVal);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_nan(abacus_ushort) {
  return nan_helper<abacus_half>();
}
abacus_half2 ABACUS_API __abacus_nan(abacus_ushort2) {
  return nan_helper<abacus_half2>();
}
abacus_half3 ABACUS_API __abacus_nan(abacus_ushort3) {
  return nan_helper<abacus_half3>();
}
abacus_half4 ABACUS_API __abacus_nan(abacus_ushort4) {
  return nan_helper<abacus_half4>();
}
abacus_half8 ABACUS_API __abacus_nan(abacus_ushort8) {
  return nan_helper<abacus_half8>();
}
abacus_half16 ABACUS_API __abacus_nan(abacus_ushort16) {
  return nan_helper<abacus_half16>();
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_nan(abacus_uint) {
  return nan_helper<abacus_float>();
}
abacus_float2 ABACUS_API __abacus_nan(abacus_uint2) {
  return nan_helper<abacus_float2>();
}
abacus_float3 ABACUS_API __abacus_nan(abacus_uint3) {
  return nan_helper<abacus_float3>();
}
abacus_float4 ABACUS_API __abacus_nan(abacus_uint4) {
  return nan_helper<abacus_float4>();
}
abacus_float8 ABACUS_API __abacus_nan(abacus_uint8) {
  return nan_helper<abacus_float8>();
}
abacus_float16 ABACUS_API __abacus_nan(abacus_uint16) {
  return nan_helper<abacus_float16>();
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_nan(abacus_ulong) {
  return nan_helper<abacus_double>();
}
abacus_double2 ABACUS_API __abacus_nan(abacus_ulong2) {
  return nan_helper<abacus_double2>();
}
abacus_double3 ABACUS_API __abacus_nan(abacus_ulong3) {
  return nan_helper<abacus_double3>();
}
abacus_double4 ABACUS_API __abacus_nan(abacus_ulong4) {
  return nan_helper<abacus_double4>();
}
abacus_double8 ABACUS_API __abacus_nan(abacus_ulong8) {
  return nan_helper<abacus_double8>();
}
abacus_double16 ABACUS_API __abacus_nan(abacus_ulong16) {
  return nan_helper<abacus_double16>();
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
