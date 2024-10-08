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

#include <abacus/abacus_common.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_extra.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

abacus_uint ABACUS_API __abacus_pack_snorm4x8(abacus_float4 x) {
  const abacus_float4 clamped =
      __abacus_clamp(x, -1.0f, 1.0f) * TypeTraits<abacus_char>::max();
  const abacus_char4 result =
      abacus::detail::cast::convert<abacus_char4>(clamped);
  return abacus::detail::cast::as<abacus_uint>(result);
}

abacus_uint ABACUS_API __abacus_pack_unorm4x8(abacus_float4 x) {
  const abacus_float4 clamped =
      __abacus_clamp(x, 0.0f, 1.0f) * TypeTraits<abacus_uchar>::max();
  const abacus_uchar4 result =
      abacus::detail::cast::convert<abacus_uchar4>(clamped);
  return abacus::detail::cast::as<abacus_uint>(result);
}

abacus_uint ABACUS_API __abacus_pack_snorm2x16(abacus_float2 x) {
  const abacus_float2 clamped =
      __abacus_clamp(x, -1.0f, 1.0f) * TypeTraits<abacus_short>::max();
  const abacus_short2 result =
      abacus::detail::cast::convert<abacus_short2>(clamped);
  return abacus::detail::cast::as<abacus_uint>(result);
}

abacus_uint ABACUS_API __abacus_pack_unorm2x16(abacus_float2 x) {
  const abacus_float2 clamped =
      __abacus_clamp(x, 0.0f, 1.0f) * TypeTraits<abacus_ushort>::max();
  const abacus_ushort2 result =
      abacus::detail::cast::convert<abacus_ushort2>(clamped);
  return abacus::detail::cast::as<abacus_uint>(result);
}

abacus_uint ABACUS_API __abacus_pack_half2x16(abacus_float2 x) {
#ifdef __CA_BUILTINS_HALF_SUPPORT
  const abacus_half2 converted = convert_half2(x);
  return abacus::detail::cast::as<abacus_uint>(converted);
#else
  const abacus_float halfMax = 65536.0f;  // Max value half can represent
  // 2^-112 magic number
  const abacus_float magic = abacus::detail::cast::as<abacus_float>(0x07800000);

  const abacus_float2 absX = __abacus_fabs(x);

  // Subtract 112 from the exponent to change the exponent bias from 127 to 15
  const abacus_float2 rangeClamper = __abacus_min(absX, halfMax) * magic;

  const abacus_int2 bitcast =
      abacus::detail::cast::as<abacus_int2>(rangeClamper);

  // Shift whole number to put exponent and mantissa in the right place
  // Float has 13 more mantissa bits of precision than half.
  const abacus_int2 nearly = bitcast >> 13;

  abacus_short2 result = abacus::detail::cast::convert<abacus_short2>(nearly);

  // Check whether the first bit that was cut off is set
  const abacus_int2 round = (bitcast & (1 << 12)) != 0;

  // Round truncated result up, if the first truncated bit was set
  result = __abacus_select(result, result + 1,
                           abacus::detail::cast::convert<abacus_short2>(round));

  const abacus_short2 c0 =
      abacus::detail::cast::convert<abacus_short2>(__abacus_isinf(x));
  result = __abacus_select(result, abacus_short2(0x7c00), c0);

  const abacus_short2 c1 =
      abacus::detail::cast::convert<abacus_short2>(__abacus_isnan(x));
  result = __abacus_select(result, abacus_short2(0x7c01), c1);

  const abacus_short2 signBit =
      abacus::detail::cast::as<abacus_short2>(abacus_ushort2(0x8000u));
  const abacus_short2 c2 =
      abacus::detail::cast::convert<abacus_short2>(__abacus_isless(x, 0.0f));
  result = __abacus_select(result, result | signBit, c2);

  return abacus::detail::cast::as<abacus_uint>(result);
#endif  // __CA_BUILTINS_HALF_SUPPORT
}

abacus_float4 ABACUS_API __abacus_unpack_snorm4x8(abacus_uint x) {
  const abacus_char4 asChar4 = abacus::detail::cast::as<abacus_char4>(x);
  const abacus_float4 asFloat =
      abacus::detail::cast::convert<abacus_float4>(asChar4);
  const abacus_float4 divisor(TypeTraits<abacus_char>::max());
  return __abacus_clamp(asFloat / divisor, -1.0f, 1.0f);
}

abacus_float4 ABACUS_API __abacus_unpack_unorm4x8(abacus_uint x) {
  const abacus_uchar4 asUChar4 = abacus::detail::cast::as<abacus_uchar4>(x);
  const abacus_float4 asFloat =
      abacus::detail::cast::convert<abacus_float4>(asUChar4);
  const abacus_float4 divisor(TypeTraits<abacus_uchar>::max());
  return asFloat / divisor;
}

abacus_float2 ABACUS_API __abacus_unpack_snorm2x16(abacus_uint x) {
  const abacus_short2 asShort2 = abacus::detail::cast::as<abacus_short2>(x);
  const abacus_float2 asFloat =
      abacus::detail::cast::convert<abacus_float2>(asShort2);
  const abacus_float2 divisor(TypeTraits<abacus_short>::max());
  return __abacus_clamp(asFloat / divisor, -1.0f, 1.0f);
}

abacus_float2 ABACUS_API __abacus_unpack_unorm2x16(abacus_uint x) {
  const abacus_ushort2 asUShort2 = abacus::detail::cast::as<abacus_ushort2>(x);
  const abacus_float2 asFloat =
      abacus::detail::cast::convert<abacus_float2>(asUShort2);
  const abacus_float2 divisor(TypeTraits<abacus_ushort>::max());
  return asFloat / divisor;
}

abacus_float2 ABACUS_API __abacus_unpack_half2x16(abacus_uint x) {
#ifdef __CA_BUILTINS_HALF_SUPPORT
  const abacus_half2 vHalf = abacus::detail::cast::as<abacus_half2>(x);
  return abacus::detail::cast::convert<abacus_float2>(vHalf);
#else
  const abacus_short2 v = abacus::detail::cast::as<abacus_short2>(x);

  const abacus_ushort2 absX =
      abacus::detail::cast::convert<abacus_ushort2>(v) & abacus_ushort(0x7FFFU);

  // Shift whole input by the 13 to convert mantissa from half (10 bits) to
  // float (23 bits)
  abacus_uint2 result = abacus::detail::cast::convert<abacus_uint2>(absX) << 13;

  // Mask for the exponent bits of a half left shifted by 13
  const abacus_uint halfExponentBits(0x0F800000);

  const abacus_uint2 exponent = result & halfExponentBits;

  const abacus_int2 infOrNan = exponent == halfExponentBits;

  // If all half exponent bits were set, the half was inf or NaN. Set the 3
  // additional exponent bits of the float to make it a inf or NaN. Otherwise,
  // add 112 to the exponent to change its bias from -15 (half) to -127 (float)
  result = __abacus_select(result + 0x38000000u, result | 0x70000000, infOrNan);

  const abacus_int2 zeroOrDenormal = exponent == 0;

  // Increase the exponent by 1 to -14, which is the exponent for denormals. The
  // denormal is then 1.mantissa * 2^-14. By substracting 1.0*2^-14 , the result
  // approximates 0.mantissa * 2^-14. If the input is zero, the result will be
  // 1.0*2^-14 - 1.0*2^-14 = 0.0.
  const abacus_uint2 denormFudge = abacus::detail::cast::as<abacus_uint2>(
      abacus::detail::cast::as<abacus_float2>(result + 0x00800000u) -
      abacus::detail::cast::as<abacus_float>(0x38800000u));

  result = __abacus_select(result, denormFudge, zeroOrDenormal);

  const abacus_int2 negativeX =
      abacus::detail::cast::convert<abacus_int2>(v) < 0;

  // Or in sign bit
  result = __abacus_select(result, result | 0x80000000u, negativeX);

  return abacus::detail::cast::as<abacus_float2>(result);
#endif  // __CA_BUILTINS_HALF_SUPPORT
}
