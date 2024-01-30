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
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/frexp_unsafe.h>
#include <abacus/internal/is_denorm.h>
namespace {

/*
 * Each type derives its own values using the base value of 2 ^ number, where
 * number is the number of bits in the mantissa plus two. All other values
 * needed can be calculated by that value. We use these template specialisations
 * to encode these values.
 */
template <typename T>
struct MagicNumbers {};

// Original comment:
/* Only need this on platforms where denorm multiplication is not supported
 multiplies a denorm number x by 2 ^ 25
 r = x * 2 ^ 25
 r = (x + c - c) * 2 ^ 25
 r = 2 ^ 25 * (x + c) - 2 ^ 25 * c
 r = 3.3554432e7 * (x + c) - 3.3554432e7 * c, c =
 1.17549435082228750796873653722E-38
 r = 3.3554432e7 * (x + 1.17549435082228750796873653722E-38) - 3.3554432e7 *
 1.17549435082228750796873653722E-38
 r = 3.3554432e7 * (x + 1.17549435082228750796873653722E-38) -
 3.94430452610505902705864282641E-31
 r = 3.3554432e7 * as_float(as_int(x) | 0x00800000) -
 3.94430452610505902705864282641E-31
*/
template <>
struct MagicNumbers<abacus_float> {
  static inline constexpr abacus_float Coeficent() { return 3.3554432e7f; }
  static inline constexpr abacus_float Subtraction() {
    return 3.944304526105059E-31f;
  }
  static inline constexpr abacus_int MultiplicationFactor() { return 25; }
};

// Original comment:
/* Only need this on platforms where denorm multiplication is not supported
 multiplies a double denorm number x by 2 ^ 54
 r = x * 2 ^ 54
 r = (x + c - c) * 2 ^ 54
 r = 2 ^ 54 * (x + c) - 2 ^ 54 * c
 r = 1.8014398509481984e16 * (x + c) - 1.8014398509481984e16 * c, c =
 2.22507385850720138309023271733e-308
 r = 1.8014398509481984e16 * (x + 2.22507385850720138309023271733e-308)
 - 1.8014398509481984e16 * 2.22507385850720138309023271733e-308 r
 = 1.8014398509481984e16 * (x + 2.22507385850720138309023271733e-308) -
 4.008336720017945555992216102695993318699958272e-292
 r = 1.8014398509481984e16 * as_double(as_long(x) | 0x0010000000000000) -
 4.008336720017945555992216102695993318699958272e-292
*/
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <>
struct MagicNumbers<abacus_double> {
  static inline constexpr abacus_double Coeficent() {
    return 1.8014398509481984e16;
  }
  static inline constexpr abacus_double Subtraction() {
    return 4.008336720017945555992216102695993318699958272e-292;
  }
  static inline constexpr abacus_int MultiplicationFactor() { return 54; }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

#ifdef __CA_BUILTINS_HALF_SUPPORT
/*
 * These numbers are derived from the above formula just like double and half.
 * MultiplicationFactor seems to be the number of mantissa bits plus two,
 * Coeficient is 2^MultiplicationFactor, and subtraction is Coeficient
 * multiplied by the number represented by just the least significant exponent
 * bit set (2^(1 - bias)).
 */
template <>
struct MagicNumbers<abacus_half> {
  static inline constexpr abacus_int MultiplicationFactor() { return 12; }
  static inline constexpr abacus_half Coeficent() { return 4096.0f16; }
  static inline constexpr abacus_half Subtraction() {
    // 0.00006103515 = least significant bit in the exponent
    // 0.00006103515 = (2^(1 - bias)) = 2^(1 - 15) = 2^-14
    return Coeficent() * 0.00006103515f16;
  }
};

#endif

template <typename T>
T frexp_denorm_multiplier(const T x) {
  using ElemType = typename TypeTraits<T>::ElementType;
  using UnsignedType = typename TypeTraits<T>::UnsignedType;
  using Shape = FPShape<T>;

  const UnsignedType xAbs =
      abacus::detail::cast::as<UnsignedType>(x) & Shape::InverseSignMask();

  const UnsignedType xAbsOrHidden = xAbs | Shape::LeastSignificantExponentBit();

  const T xAbsPlusHidden = abacus::detail::cast::as<T>(xAbsOrHidden);

  const T result = xAbsPlusHidden * MagicNumbers<ElemType>::Coeficent() -
                   MagicNumbers<ElemType>::Subtraction();

  return __abacus_copysign(result, x);
}

template <typename T>
T frexp(const T x,
        typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type
            *out_exponent) {
  typedef typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type
      IntVecType;
  typedef typename TypeTraits<T>::SignedType SignedType;

  using ElemType = typename TypeTraits<T>::ElementType;

  const SignedType is_denorm = abacus::internal::is_denorm(x);

  const IntVecType sub = MagicNumbers<ElemType>::MultiplicationFactor();
  const IntVecType offset = __abacus_select(
      (IntVecType)0, sub, abacus::detail::cast::convert<IntVecType>(is_denorm));
  const T newX = __abacus_select(x, frexp_denorm_multiplier(x), is_denorm);

  IntVecType exponent;
  const T result = abacus::internal::frexp_unsafe(newX, &exponent);

  // frexp should still work on denormals, even in FTZ mode, however comparing
  // a denormal number to 0 in FTZ mode will yield true, so we don't take into
  // account this comparison if the number is denormal. It also works fine
  // with denormal support since 0 is not denormal.
  const SignedType cond = (((__abacus_fabs(x) == 0.0f) & ~is_denorm) |
                           __abacus_isinf(x) | __abacus_isnan(x));
  *out_exponent = __abacus_select(
      exponent - offset, 0, abacus::detail::cast::convert<IntVecType>(cond));
  return __abacus_select(result, x, cond);
}
}  // namespace

abacus_float ABACUS_API __abacus_frexp(abacus_float x, abacus_int *o) {
  return frexp<>(x, o);
}
abacus_float2 ABACUS_API __abacus_frexp(abacus_float2 x, abacus_int2 *o) {
  return frexp<>(x, o);
}
abacus_float3 ABACUS_API __abacus_frexp(abacus_float3 x, abacus_int3 *o) {
  return frexp<>(x, o);
}
abacus_float4 ABACUS_API __abacus_frexp(abacus_float4 x, abacus_int4 *o) {
  return frexp<>(x, o);
}
abacus_float8 ABACUS_API __abacus_frexp(abacus_float8 x, abacus_int8 *o) {
  return frexp<>(x, o);
}
abacus_float16 ABACUS_API __abacus_frexp(abacus_float16 x, abacus_int16 *o) {
  return frexp<>(x, o);
}
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_frexp(abacus_double x, abacus_int *o) {
  return frexp<>(x, o);
}
abacus_double2 ABACUS_API __abacus_frexp(abacus_double2 x, abacus_int2 *o) {
  return frexp<>(x, o);
}
abacus_double3 ABACUS_API __abacus_frexp(abacus_double3 x, abacus_int3 *o) {
  return frexp<>(x, o);
}
abacus_double4 ABACUS_API __abacus_frexp(abacus_double4 x, abacus_int4 *o) {
  return frexp<>(x, o);
}
abacus_double8 ABACUS_API __abacus_frexp(abacus_double8 x, abacus_int8 *o) {
  return frexp<>(x, o);
}
abacus_double16 ABACUS_API __abacus_frexp(abacus_double16 x, abacus_int16 *o) {
  return frexp<>(x, o);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

#ifdef __CA_BUILTINS_HALF_SUPPORT

abacus_half ABACUS_API __abacus_frexp(abacus_half x, abacus_int *o) {
  return frexp<>(x, o);
}
abacus_half2 ABACUS_API __abacus_frexp(abacus_half2 x, abacus_int2 *o) {
  return frexp<>(x, o);
}
abacus_half3 ABACUS_API __abacus_frexp(abacus_half3 x, abacus_int3 *o) {
  return frexp<>(x, o);
}
abacus_half4 ABACUS_API __abacus_frexp(abacus_half4 x, abacus_int4 *o) {
  return frexp<>(x, o);
}
abacus_half8 ABACUS_API __abacus_frexp(abacus_half8 x, abacus_int8 *o) {
  return frexp<>(x, o);
}
abacus_half16 ABACUS_API __abacus_frexp(abacus_half16 x, abacus_int16 *o) {
  return frexp<>(x, o);
}

#endif
