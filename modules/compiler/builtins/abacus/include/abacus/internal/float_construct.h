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

#ifndef __ABACUS_INTERNAL_FLOAT_CONSTRUCT_H__
#define __ABACUS_INTERNAL_FLOAT_CONSTRUCT_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_integer.h>
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T, typename UInt = typename TypeTraits<T>::UnsignedType>
inline T construct_helper_scalar(UInt mantissa, UInt unBiasedExp) {
  typedef FPShape<T> Shape;

  if (0 == mantissa) {
    return (T)0;
  }

  // how many places we need to shift up to nomalise the mantissa
  const UInt shift = __abacus_clz(mantissa) - Shape::Exponent();
  const UInt mantshift = __abacus_min(shift, unBiasedExp);

  // shift mantissa by this
  mantissa <<= mantshift;
  unBiasedExp -= mantshift;

  // if the exponent is 0 the answer is a denorm so we shift down by one.
  mantissa >>= (0 == unBiasedExp) ? 1 : 0;

  const UInt ansuint =
      (unBiasedExp << Shape::Mantissa()) | (mantissa & Shape::MantissaMask());

  return abacus::detail::cast::as<T>(ansuint);
}

template <typename T, typename UInt = typename TypeTraits<T>::UnsignedType>
inline T construct_helper_vector(UInt mantissa, UInt unBiasedExp) {
  typedef typename TypeTraits<T>::SignedType SInt;
  typedef FPShape<T> Shape;

  // how many places we need to shift up to nomalise the mantissa
  const UInt shift = __abacus_clz(mantissa) - Shape::Exponent();
  const UInt mantshift = __abacus_min(shift, unBiasedExp);

  // shift mantissa by this
  mantissa <<= mantshift;
  unBiasedExp -= mantshift;

  // if the exponent is 0 the answer is a denorm so we shift down by one.
  const SInt shiftBy = (unBiasedExp == 0);
  mantissa >>= abacus::detail::cast::convert<UInt>(-shiftBy);

  UInt ansuint =
      (unBiasedExp << Shape::Mantissa()) | (mantissa & Shape::MantissaMask());

  const SInt isZero = (mantissa == 0);
  ansuint = __abacus_select(ansuint, (UInt)0, isZero);

  return abacus::detail::cast::as<T>(ansuint);
}
}  // namespace

namespace abacus {
namespace internal {

#ifdef __CA_BUILTINS_HALF_SUPPORT
inline abacus_half float_construct(abacus_ushort mantissa,
                                   abacus_ushort unBiasedExp) {
  return construct_helper_scalar<abacus_half>(mantissa, unBiasedExp);
}

inline abacus_half2 float_construct(abacus_ushort2 mantissa,
                                    abacus_ushort2 unBiasedExp) {
  return construct_helper_vector<abacus_half2>(mantissa, unBiasedExp);
}

inline abacus_half3 float_construct(abacus_ushort3 mantissa,
                                    abacus_ushort3 unBiasedExp) {
  return construct_helper_vector<abacus_half3>(mantissa, unBiasedExp);
}

inline abacus_half4 float_construct(abacus_ushort4 mantissa,
                                    abacus_ushort4 unBiasedExp) {
  return construct_helper_vector<abacus_half4>(mantissa, unBiasedExp);
}

inline abacus_half8 float_construct(abacus_ushort8 mantissa,
                                    abacus_ushort8 unBiasedExp) {
  return construct_helper_vector<abacus_half8>(mantissa, unBiasedExp);
}

inline abacus_half16 float_construct(abacus_ushort16 mantissa,
                                     abacus_ushort16 unBiasedExp) {
  return construct_helper_vector<abacus_half16>(mantissa, unBiasedExp);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

inline abacus_float float_construct(abacus_uint mantissa,
                                    abacus_uint unBiasedExp) {
  return construct_helper_scalar<abacus_float>(mantissa, unBiasedExp);
}

inline abacus_float2 float_construct(abacus_uint2 mantissa,
                                     abacus_uint2 unBiasedExp) {
  return construct_helper_vector<abacus_float2>(mantissa, unBiasedExp);
}

inline abacus_float3 float_construct(abacus_uint3 mantissa,
                                     abacus_uint3 unBiasedExp) {
  return construct_helper_vector<abacus_float3>(mantissa, unBiasedExp);
}

inline abacus_float4 float_construct(abacus_uint4 mantissa,
                                     abacus_uint4 unBiasedExp) {
  return construct_helper_vector<abacus_float4>(mantissa, unBiasedExp);
}

inline abacus_float8 float_construct(abacus_uint8 mantissa,
                                     abacus_uint8 unBiasedExp) {
  return construct_helper_vector<abacus_float8>(mantissa, unBiasedExp);
}

inline abacus_float16 float_construct(abacus_uint16 mantissa,
                                      abacus_uint16 unBiasedExp) {
  return construct_helper_vector<abacus_float16>(mantissa, unBiasedExp);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
inline abacus_double float_construct(abacus_ulong mantissa,
                                     abacus_ulong unBiasedExp) {
  return construct_helper_scalar<abacus_double>(mantissa, unBiasedExp);
}

inline abacus_double2 float_construct(abacus_ulong2 mantissa,
                                      abacus_ulong2 unBiasedExp) {
  return construct_helper_vector<abacus_double2>(mantissa, unBiasedExp);
}

inline abacus_double3 float_construct(abacus_ulong3 mantissa,
                                      abacus_ulong3 unBiasedExp) {
  return construct_helper_vector<abacus_double3>(mantissa, unBiasedExp);
}

inline abacus_double4 float_construct(abacus_ulong4 mantissa,
                                      abacus_ulong4 unBiasedExp) {
  return construct_helper_vector<abacus_double4>(mantissa, unBiasedExp);
}

inline abacus_double8 float_construct(abacus_ulong8 mantissa,
                                      abacus_ulong8 unBiasedExp) {
  return construct_helper_vector<abacus_double8>(mantissa, unBiasedExp);
}

inline abacus_double16 float_construct(abacus_ulong16 mantissa,
                                       abacus_ulong16 unBiasedExp) {
  return construct_helper_vector<abacus_double16>(mantissa, unBiasedExp);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_FLOAT_CONSTRUCT_H__
