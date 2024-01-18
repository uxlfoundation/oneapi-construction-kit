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

#ifndef __ABACUS_INTERNAL_FLOAT_DECONSTRUCT_H__
#define __ABACUS_INTERNAL_FLOAT_DECONSTRUCT_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T, typename UInt = typename TypeTraits<T>::UnsignedType>
inline UInt deconstruct_helper(T x, UInt *unBiasedExp) {
  using Shape = FPShape<T>;

  const UInt ix = abacus::detail::cast::as<UInt>(x);
  const UInt exponent = ix >> Shape::Mantissa();
  const UInt mantissa =
      (ix & Shape::MantissaMask()) | Shape::LeastSignificantExponentBit();

  *unBiasedExp = exponent;

  const typename TypeTraits<T>::SignedType zeroExp = (UInt)0 == exponent;
  const UInt shift = ix << 1;
  return __abacus_select(mantissa, shift, zeroExp);
}
}  // namespace

namespace abacus {
namespace internal {

#ifdef __CA_BUILTINS_HALF_SUPPORT
inline abacus_ushort float_deconstruct(abacus_half x,
                                       abacus_ushort *unBiasedExp) {
  return deconstruct_helper<abacus_half>(x, unBiasedExp);
}

inline abacus_ushort2 float_deconstruct(abacus_half2 x,
                                        abacus_ushort2 *unBiasedExp) {
  return deconstruct_helper<abacus_half2>(x, unBiasedExp);
}

inline abacus_ushort3 float_deconstruct(abacus_half3 x,
                                        abacus_ushort3 *unBiasedExp) {
  return deconstruct_helper<abacus_half3>(x, unBiasedExp);
}

inline abacus_ushort4 float_deconstruct(abacus_half4 x,
                                        abacus_ushort4 *unBiasedExp) {
  return deconstruct_helper<abacus_half4>(x, unBiasedExp);
}

inline abacus_ushort8 float_deconstruct(abacus_half8 x,
                                        abacus_ushort8 *unBiasedExp) {
  return deconstruct_helper<abacus_half8>(x, unBiasedExp);
}

inline abacus_ushort16 float_deconstruct(abacus_half16 x,
                                         abacus_ushort16 *unBiasedExp) {
  return deconstruct_helper<abacus_half16>(x, unBiasedExp);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

inline abacus_uint float_deconstruct(abacus_float x, abacus_uint *unBiasedExp) {
  return deconstruct_helper<abacus_float>(x, unBiasedExp);
}

inline abacus_uint2 float_deconstruct(abacus_float2 x,
                                      abacus_uint2 *unBiasedExp) {
  return deconstruct_helper<abacus_float2>(x, unBiasedExp);
}

inline abacus_uint3 float_deconstruct(abacus_float3 x,
                                      abacus_uint3 *unBiasedExp) {
  return deconstruct_helper<abacus_float3>(x, unBiasedExp);
}

inline abacus_uint4 float_deconstruct(abacus_float4 x,
                                      abacus_uint4 *unBiasedExp) {
  return deconstruct_helper<abacus_float4>(x, unBiasedExp);
}

inline abacus_uint8 float_deconstruct(abacus_float8 x,
                                      abacus_uint8 *unBiasedExp) {
  return deconstruct_helper<abacus_float8>(x, unBiasedExp);
}

inline abacus_uint16 float_deconstruct(abacus_float16 x,
                                       abacus_uint16 *unBiasedExp) {
  return deconstruct_helper<abacus_float16>(x, unBiasedExp);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
inline abacus_ulong float_deconstruct(abacus_double x,
                                      abacus_ulong *unBiasedExp) {
  return deconstruct_helper<abacus_double>(x, unBiasedExp);
}

inline abacus_ulong2 float_deconstruct(abacus_double2 x,
                                       abacus_ulong2 *unBiasedExp) {
  return deconstruct_helper<abacus_double2>(x, unBiasedExp);
}

inline abacus_ulong3 float_deconstruct(abacus_double3 x,
                                       abacus_ulong3 *unBiasedExp) {
  return deconstruct_helper<abacus_double3>(x, unBiasedExp);
}

inline abacus_ulong4 float_deconstruct(abacus_double4 x,
                                       abacus_ulong4 *unBiasedExp) {
  return deconstruct_helper<abacus_double4>(x, unBiasedExp);
}

inline abacus_ulong8 float_deconstruct(abacus_double8 x,
                                       abacus_ulong8 *unBiasedExp) {
  return deconstruct_helper<abacus_double8>(x, unBiasedExp);
}

inline abacus_ulong16 float_deconstruct(abacus_double16 x,
                                        abacus_ulong16 *unBiasedExp) {
  return deconstruct_helper<abacus_double16>(x, unBiasedExp);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_FLOAT_DECONSTRUCT_H__
