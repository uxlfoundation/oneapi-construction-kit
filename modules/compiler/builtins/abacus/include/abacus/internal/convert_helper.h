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
#ifndef __ABACUS_CAST_CONVERT_HELPER_H__
#define __ABACUS_CAST_CONVERT_HELPER_H__

#include <abacus/abacus_common.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_integer.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/floating_point.h>

namespace {

template <typename T, typename U>
struct BitsToKeepHelper {
  static typename TypeTraits<T>::UnsignedType _(const T &t) {
    using UnsignedType = typename TypeTraits<T>::UnsignedType;
    const UnsignedType leadingZeros = __abacus_clz(__abacus_abs(t));
    return leadingZeros + FPShape<U>::Mantissa() + 1;
  }
};

// Helpers for the different rounding modes:
// RTE rounding mode
struct RTEHelper {
  template <typename T, typename U>
  static T FloatingPointToInteger(const U &u) {
    return abacus::detail::cast::convert<T>(__abacus_rint(u));
  }

  template <typename T, typename U>
  static T LargeIntegerToFloatingPoint(const U &u) {
    return abacus::detail::cast::convert<T>(u);
  }

  template <typename FP, typename UnsignedType>
  static FP RoundNearInfinity(UnsignedType sign) {
    FP out;
    out.Mantissa = 0;
    out.Exponent = FP::Shape::ExponentOnes();
    out.Sign = abacus::detail::cast::convert<typename FP::UnsignedType>(sign);

    return out;
  }

  template <typename UnsignedType>
  static UnsignedType ShiftRightLogical(UnsignedType x, UnsignedType shift,
                                        UnsignedType numBits, UnsignedType) {
    UnsignedType round = x & ((UnsignedType(1) << shift) - UnsignedType(1));
    UnsignedType shifted = x >> shift;

    // exactly between two numbers
    UnsignedType result = __abacus_select(
        shifted, shifted + UnsignedType(1),
        abacus::detail::cast::convert<UnsignedType>((shifted & 0x1) == 1));

    // closer to the number below
    result = __abacus_select(result, shifted,
                             abacus::detail::cast::convert<UnsignedType>(
                                 round < (UnsignedType(1) << (shift - 1))));

    // closer to the number above
    result = __abacus_select(result, shifted + UnsignedType(1),
                             abacus::detail::cast::convert<UnsignedType>(
                                 round > (UnsignedType(1) << (shift - 1))));

    result = __abacus_select(
        result, UnsignedType(0),
        abacus::detail::cast::convert<UnsignedType>(shift > numBits));
    result = __abacus_select(
        result, x >> (numBits - 1),
        abacus::detail::cast::convert<UnsignedType>(shift == numBits));

    return result;
  }
};

// RTN rounding mode
struct RTNHelper {
  template <typename T, typename U>
  static T FloatingPointToInteger(const U &u) {
    return abacus::detail::cast::convert<T>(__abacus_floor(u));
  }

  template <typename T, typename U>
  static T LargeIntegerToFloatingPoint(const U &u) {
    using ElementType = typename TypeTraits<U>::ElementType;
    using UnsignedType = typename TypeTraits<U>::UnsignedType;
    const UnsignedType to_keep = BitsToKeepHelper<U, T>::_(u);

    // shift payload, trimming it to to_keep bits, return amount shifted
    const UnsignedType sz = 8 * sizeof(ElementType);
    const U to_drop =
        abacus::detail::cast::convert<U>(sz - __abacus_min(sz, to_keep));
    const U new_payload = u >> to_drop;

    return abacus::detail::cast::convert<T>(U(1) << to_drop) *
           abacus::detail::cast::convert<T>(new_payload);
  }

  template <typename FP, typename UnsignedType>
  static FP RoundNearInfinity(UnsignedType s) {
    using FPUnsigned = typename FP::UnsignedType;
    using FPSigned = typename FP::SignedType;
    FP out;

    const FPSigned sign = abacus::detail::cast::convert<FPUnsigned>(s) == 1;

    out.Mantissa =
        __abacus_select(FP::Shape::MantissaOnes(), FPUnsigned(0), sign);
    out.Exponent = __abacus_select(FPUnsigned(FP::Shape::ExponentOnes() - 1u),
                                   FP::Shape::ExponentOnes(), sign);
    out.Sign = abacus::detail::cast::convert<FPUnsigned>(s);

    return out;
  }

  template <typename UnsignedType>
  static UnsignedType ShiftRightLogical(UnsignedType x, UnsignedType shift,
                                        UnsignedType numBits,
                                        UnsignedType sign) {
    const UnsignedType round =
        x & ((UnsignedType(1) << shift) - UnsignedType(1));
    const UnsignedType large =
        abacus::detail::cast::convert<UnsignedType>(shift >= numBits);
    const UnsignedType shifted = x >> shift;

    UnsignedType result = shifted + sign;
    result = __abacus_select(
        result, shifted,
        abacus::detail::cast::convert<UnsignedType>(round == 0));
    result = __abacus_select(result, sign, large);

    return result;
  }
};

// RTP rounding mode
struct RTPHelper {
  template <typename T, typename U>
  static T FloatingPointToInteger(const U &u) {
    return abacus::detail::cast::convert<T>(__abacus_ceil(u));
  }

  template <typename T, typename U>
  static T LargeIntegerToFloatingPoint(const U &payload) {
    using ElementType = typename TypeTraits<U>::ElementType;
    using UnsignedType = typename TypeTraits<U>::UnsignedType;
    const UnsignedType to_keep = BitsToKeepHelper<U, T>::_(payload);

    // shift payload, trimming it to to_keep bits, return amount shifted
    const UnsignedType sz = 8 * sizeof(ElementType);
    const UnsignedType to_drop = sz - __abacus_min(sz, to_keep);
    const UnsignedType mask = ~(TypeTraits<UnsignedType>::max() << to_drop);

    const U signed_mask = abacus::detail::cast::as<U>(mask);
    const U signed_to_drop =
        abacus::detail::cast::convert<U>(sz - __abacus_min(sz, to_keep));

    const U to_add = ((payload & signed_mask) + signed_mask) >> signed_to_drop;
    const U new_payload = (payload >> signed_to_drop) + to_add;
    return abacus::detail::cast::convert<T>(U(1) << signed_to_drop) *
           abacus::detail::cast::convert<T>(new_payload);
  }

  template <typename FP, typename UnsignedType>
  static FP RoundNearInfinity(UnsignedType s) {
    using FPUnsigned = typename FP::UnsignedType;
    using FPSigned = typename FP::SignedType;
    FP out;

    const FPSigned sign = abacus::detail::cast::convert<FPUnsigned>(s) == 1;

    out.Mantissa = __abacus_select(FPUnsigned(0),
                                   FPUnsigned(FP::Shape::MantissaOnes()), sign);
    out.Exponent = abacus::detail::cast::convert<FPUnsigned>(
        (UnsignedType)FP::Shape::ExponentOnes() - s);
    out.Sign = abacus::detail::cast::convert<FPUnsigned>(s);

    return out;
  }

  template <typename UnsignedType>
  static UnsignedType ShiftRightLogical(UnsignedType x, UnsignedType shift,
                                        UnsignedType numBits,
                                        UnsignedType sign) {
    const UnsignedType round =
        x & ((UnsignedType(1) << shift) - UnsignedType(1));
    const UnsignedType large =
        abacus::detail::cast::convert<UnsignedType>(shift >= numBits);
    const UnsignedType shifted = x >> shift;
    const UnsignedType signdelta = __abacus_select(
        UnsignedType(1), UnsignedType(0),
        abacus::detail::cast::convert<UnsignedType>(sign == UnsignedType(1)));

    UnsignedType result = shifted + signdelta;
    result = __abacus_select(
        result, shifted,
        abacus::detail::cast::convert<UnsignedType>(round == 0));
    result = __abacus_select(result, signdelta, large);

    return result;
  }
};

// RTZ rounding mode
struct RTZHelper {
  template <typename T, typename U>
  static T FloatingPointToInteger(const U &u) {
    return abacus::detail::cast::convert<T>(__abacus_trunc(u));
  }

  template <typename T, typename U>
  static T LargeIntegerToFloatingPoint(const U &payload) {
    using ElementType = typename TypeTraits<U>::ElementType;
    using UnsignedType = typename TypeTraits<U>::UnsignedType;
    const UnsignedType to_keep = BitsToKeepHelper<U, T>::_(payload);

    // shift payload, trimming it to to_keep bits, return amount shifted
    const UnsignedType sz = 8 * sizeof(ElementType);
    const UnsignedType to_drop = sz - __abacus_min(sz, to_keep);
    const U signed_to_drop = abacus::detail::cast::convert<U>(to_drop);
    U new_payload;
    if (TypeTraits<U>::is_signed) {
      const UnsignedType mask = ~(TypeTraits<UnsignedType>::max() << to_drop);
      const U add = U(payload >> abacus::detail::cast::convert<U>(sz - 1)) &
                    abacus::detail::cast::as<U>(mask);
      new_payload = U(payload + add) >> signed_to_drop;
    } else {
      new_payload = payload >> signed_to_drop;
    }

    return abacus::detail::cast::convert<T>(U(1) << signed_to_drop) *
           abacus::detail::cast::convert<T>(new_payload);
  }

  template <typename FP, typename UnsignedType>
  static FP RoundNearInfinity(UnsignedType sign) {
    FP out;
    out.Mantissa = FP::Shape::MantissaOnes();
    out.Exponent = FP::Shape::ExponentOnes() - 1u;
    out.Sign = abacus::detail::cast::convert<typename FP::UnsignedType>(sign);

    return out;
  }

  template <typename UnsignedType>
  static UnsignedType ShiftRightLogical(UnsignedType x, UnsignedType shift,
                                        UnsignedType numBits, UnsignedType) {
    UnsignedType result;

    result = x >> shift;

    // Shifting >#bits trivially results in a zero output.  Shifting by exactly
    // #bits requires preserving the MSB for rounding purposes (even though it
    // should get discarded by the shift).
    result = __abacus_select(
        result, UnsignedType(0),
        abacus::detail::cast::convert<UnsignedType>(shift > numBits));
    result = __abacus_select(
        result, x >> (numBits - 1u),
        abacus::detail::cast::convert<UnsignedType>(shift == numBits));

    return result;
  }
};

// Conversion from a larger floating point type to a smaller one
template <typename T, typename U, typename H>
inline T DownFloatConvertHelper(const U payload) {
  using FPT = abacus::internal::FloatingPoint<T>;
  using FPU = abacus::internal::FloatingPoint<U>;
  using UnsignedType = typename TypeTraits<U>::UnsignedType;
  using SignedType = typename TypeTraits<U>::SignedType;
  using TUnsignedType = typename TypeTraits<T>::UnsignedType;

  const FPU in(payload);
  // Intermediary FloatingPoint of the larger size used for calculations and
  // downsized to the smaller size at the end.
  FPU wip;

  // We create the smaller type Floating point here to use the associated Shape
  // informations.
  FPT out;

  // Exponent biases for the floating point types.
  const SignedType outBias =
      abacus::detail::cast::convert<SignedType>(out.Bias());
  const SignedType inBias = in.Bias();

  // conditions
  const SignedType from_zero = in.Zero();
  const SignedType from_nan = in.NaN();
  const SignedType from_inf = in.Inf();
  const SignedType to_denorm =
      (abacus::detail::cast::as<SignedType>(in.Exponent) + outBias) <= inBias;

  // Calculation for normal numbers:
  //
  // This input is just a normal number, scale appropriately.
  // Mantissa: Scale down using the given rounding mode helper.
  // Exponent: copy, but adjust for the difference in bias.
  UnsignedType shift = FPU::Shape::Mantissa() - FPT::Shape::Mantissa();
  UnsignedType mantissa = H::template ShiftRightLogical<UnsignedType>(
      in.Mantissa, shift, FPU::Shape::NumBits(), in.Sign);
  UnsignedType exponent = abacus::detail::cast::as<UnsignedType>(
      abacus::detail::cast::as<SignedType>(in.Exponent) + outBias - inBias);

  // If the mantissa has been rounded up to only the hidden bit, it won't fit in
  // the smaller type's mantissa, so we round up to the next representable
  // number (i.e. bump the exponent).
  SignedType rounded_mantissa =
      mantissa == (FPT::Shape::ONE() << FPT::Shape::Mantissa());
  mantissa = __abacus_select(mantissa, UnsignedType(0), rounded_mantissa);
  exponent = __abacus_select(exponent, exponent + 1, rounded_mantissa);

  wip.Mantissa = mantissa;
  wip.Exponent = exponent;
  wip.Sign = in.Sign;

  // We may have produced a number larger than the largest representable
  // number, numerically this will have been calculated as a too large
  // exponent (e.g. overlapping with the inf/NaN representation).  What to
  // do here depends on the rounding mode, but the options are either to go
  // with an infinite or round down to the largest representable value.
  const SignedType rni_cond = exponent >= FPT::Shape::ExponentOnes();
  const FPT outrni = H::template RoundNearInfinity<FPT, UnsignedType>(in.Sign);
  wip.Mantissa = __abacus_select(
      wip.Mantissa,
      abacus::detail::cast::convert<UnsignedType>(outrni.Mantissa), rni_cond);
  wip.Exponent = __abacus_select(
      wip.Exponent,
      abacus::detail::cast::convert<UnsignedType>(outrni.Exponent), rni_cond);
  wip.Sign = __abacus_select(
      wip.Sign, abacus::detail::cast::convert<UnsignedType>(outrni.Sign),
      rni_cond);

  // Calculation for denormals and numbers that become denormal
  //
  // Unlike with normal numbers, the value of the input exponent affects the
  // output mantissa for denormal numbers.  So scale the mantissa by both the
  // difference in bits available, and the (biased) input exponent.
  //
  // Note: We add 1 to the shift to pair with setting the 24th or 53rd bit in
  // the mantissa before shifting below.
  const UnsignedType bias_exponent = abacus::detail::cast::as<UnsignedType>(
      abacus::detail::cast::as<SignedType>(in.Exponent) + outBias - inBias);
  shift = __abacus_select(shift, shift - bias_exponent + UnsignedType(1),
                          to_denorm);

  // The mantissa is produced by shifting using the relevant rounding mode
  // helper.
  //
  // Note: Before shifting the input mantissa we set the next bit to one,
  // e.g. a 32-bit float has a 23-bit mantissa, so we set the 24th bit (1u <<
  // 23).  Countering this is why we added one to the shift above, but by
  // doing this we ensure that we round correctly for denormals (i.e. this is
  // not required for the normal case).
  mantissa = H::template ShiftRightLogical<UnsignedType>(
      in.Mantissa | (FPU::Shape::ONE() << FPU::Shape::Mantissa()), shift,
      FPU::Shape::NumBits(), in.Sign);

  // If the mantissa has been rounded up to only the hidden bit, it won't fit in
  // the smaller type's mantissa, so we round up to the next representable
  // number (i.e. bump the exponent).
  rounded_mantissa = mantissa == (FPT::Shape::ONE() << FPT::Shape::Mantissa());
  mantissa = __abacus_select(mantissa, UnsignedType(0), rounded_mantissa);
  exponent =
      __abacus_select(UnsignedType(0), UnsignedType(1), rounded_mantissa);

  wip.Mantissa = __abacus_select(wip.Mantissa, mantissa, to_denorm);
  wip.Exponent = __abacus_select(wip.Exponent, exponent, to_denorm);
  wip.Sign = __abacus_select(wip.Sign, in.Sign, to_denorm);

  // From Zero:
  wip.Mantissa = __abacus_select(wip.Mantissa, UnsignedType(0), from_zero);
  wip.Exponent = __abacus_select(wip.Exponent, UnsignedType(0), from_zero);
  wip.Sign = __abacus_select(wip.Sign, in.Sign, from_zero);

  // From Inf:
  //
  // The input is an inf, so the output is an inf.  Set all exponent bits of
  // output to 1, and all mantissa bits to 0, preserve sign.
  wip.Mantissa = __abacus_select(wip.Mantissa, UnsignedType(0), from_inf);
  wip.Exponent = __abacus_select(
      wip.Exponent, UnsignedType(FPT::Shape::ExponentOnes()), from_inf);
  wip.Sign = __abacus_select(wip.Sign, in.Sign, from_inf);

  // From NaN:
  //
  // The input is a NaN, so the output is a NaN.  We preserve the upper
  // mantissa bits in case they are used for signalling, but we force the
  // lower bit on to ensure we never have a zero mantissa (which would be
  // interpreted as an infinity).  Set all exponent bits of output to 1, and
  // preserve sign.
  shift = FPU::Shape::Mantissa() - FPT::Shape::Mantissa();
  wip.Mantissa =
      __abacus_select(wip.Mantissa, (in.Mantissa >> shift) | 0x1, from_nan);
  wip.Exponent = __abacus_select(
      wip.Exponent, UnsignedType(FPT::Shape::ExponentOnes()), from_nan);
  wip.Sign = __abacus_select(wip.Sign, in.Sign, from_nan);

  // downscale the unsigned integer to the output type UnsignedType
  out.Mantissa = abacus::detail::cast::convert<TUnsignedType>(wip.Mantissa);
  out.Exponent = abacus::detail::cast::convert<TUnsignedType>(wip.Exponent);
  out.Sign = abacus::detail::cast::convert<TUnsignedType>(wip.Sign);

  // construct the floating point and return it
  return out.Get();
}

// conversion helpers:
// for the default rounding mode there's nothing to do
template <typename T, typename U>
struct DefaultConvertHelper {
  static T _(const U &u) { return abacus::detail::cast::convert<T>(u); }
};

enum ToInt : bool {
  IsToInt = true,
  NotToInt = false,
};

enum FromInt : bool {
  IsFromInt = true,
  NotFromInt = false,
};

enum Upscale : bool {
  IsUpscale = true,
  NotUpscale = false,
};

// for other rounding modes it depends on the source and target types
template <typename T, typename U, typename H,
          ToInt = ToInt(TypeTraits<T>::is_int),
          FromInt = FromInt(TypeTraits<U>::is_int),
          Upscale = Upscale(sizeof(typename TypeTraits<T>::ElementType) >
                            sizeof(typename TypeTraits<U>::ElementType))>
struct ConvertHelper {
  static T _(const U &u) { return abacus::detail::cast::convert<T>(u); }
};

// T -> T (floating points)
template <typename T, typename H>
struct ConvertHelper<T, T, H, NotToInt, NotFromInt, NotUpscale> {
  static T _(const T &u) { return u; }
};

// T -> T (integer type)
template <typename T, typename H>
struct ConvertHelper<T, T, H, IsToInt, IsFromInt, NotUpscale> {
  static T _(const T &u) { return u; }
};

// floating point n -> Integer type n
template <typename T, typename U, typename H, Upscale B>
struct ConvertHelper<T, U, H, IsToInt, NotFromInt, B> {
  static T _(const U &u) { return H::template FloatingPointToInteger<T, U>(u); }
};

// larger integer type n -> floatn or doublen
template <typename T, typename U, typename H>
struct ConvertHelper<T, U, H, NotToInt, IsFromInt, NotUpscale> {
  static T _(const U &payload) {
    return H::template LargeIntegerToFloatingPoint<T, U>(payload);
  }
};

// floatn -> doublen, halfn -> floatn
template <typename T, typename U, typename H>
struct ConvertHelper<T, U, H, NotToInt, NotFromInt, IsUpscale> {
  static T _(const U &u) { return abacus::detail::cast::convert<T>(u); }
};

// doublen -> floatn, floatn -> halfn
template <typename T, typename U, typename H>
struct ConvertHelper<T, U, H, NotToInt, NotFromInt, NotUpscale> {
  static T _(const U &u) { return DownFloatConvertHelper<T, U, H>(u); }
};

enum ToSigned : bool {
  IsToSigned = true,
  NotToSigned = false,
};

enum FromSigned : bool {
  IsFromSigned = true,
  NotFromSigned = false,
};

// helper to saturate conversions between integer types
template <typename T, typename U, ToSigned = ToSigned(TypeTraits<T>::is_signed),
          FromSigned = FromSigned(TypeTraits<U>::is_signed)>
struct SatIntHelper;

// T unsigned, U unsigned
template <typename T, typename U>
struct SatIntHelper<T, U, NotToSigned, NotFromSigned> {
  static U _(const U &u) {
    using TElement = typename TypeTraits<T>::ElementType;
    using UElement = typename TypeTraits<U>::ElementType;

    U temp(u);

    if (sizeof(TElement) < sizeof(UElement)) {
      const UElement max = static_cast<UElement>(TypeTraits<TElement>::max());
      temp = __abacus_min(u, max);
    }

    return temp;
  }
};

// T unsigned, U signed
template <typename T, typename U>
struct SatIntHelper<T, U, NotToSigned, IsFromSigned> {
  static U _(const U &u) {
    using TElement = typename TypeTraits<T>::ElementType;
    using UElement = typename TypeTraits<U>::ElementType;

    U temp(u);

    if (sizeof(TElement) < sizeof(UElement)) {
      const UElement min = static_cast<UElement>(TypeTraits<TElement>::min());
      const UElement max = static_cast<UElement>(TypeTraits<TElement>::max());
      temp = __abacus_clamp(u, min, max);
    } else {
      // because T is unsigned, and U is signed, we need to clamp a
      // potentially negative number in U to 0
      temp = __abacus_max(u, UElement(0));
    }

    return temp;
  }
};

// T signed, U unsigned
template <typename T, typename U>
struct SatIntHelper<T, U, IsToSigned, NotFromSigned> {
  static U _(const U &u) {
    using TElement = typename TypeTraits<T>::ElementType;
    using UElement = typename TypeTraits<U>::ElementType;

    U temp(u);

    if (sizeof(TElement) <= sizeof(UElement)) {
      // T is signed, U is unsigned and types are the same size.
      // Need to clamp a too large unsigned input.
      const UElement max = static_cast<UElement>(TypeTraits<TElement>::max());
      temp = __abacus_min(u, max);
    }

    return temp;
  }
};

// T signed, U signed
template <typename T, typename U>
struct SatIntHelper<T, U, IsToSigned, IsFromSigned> {
  static U _(const U &u) {
    using TElement = typename TypeTraits<T>::ElementType;
    using UElement = typename TypeTraits<U>::ElementType;

    U temp(u);

    if (sizeof(TElement) < sizeof(UElement)) {
      const UElement min = static_cast<UElement>(TypeTraits<TElement>::min());
      const UElement max = static_cast<UElement>(TypeTraits<TElement>::max());
      temp = __abacus_clamp(u, min, max);
    }

    return temp;
  }
};

// convert from U to T with saturation and the rounding mode C
template <typename T, typename U, typename C,
          typename E = typename TypeTraits<U>::ElementType,
          FromInt = FromInt(TypeTraits<U>::is_int),
          ToInt = ToInt(TypeTraits<T>::is_int),
          Upscale = Upscale(sizeof(typename TypeTraits<T>::ElementType) >
                            sizeof(typename TypeTraits<U>::ElementType))>
struct convert_sat_choice;

// T and U are integer types
template <typename T, typename U, typename C, typename E, Upscale D>
struct convert_sat_choice<T, U, C, E, IsFromInt, IsToInt, D> {
  static T _(const U &u) { return C::_(SatIntHelper<T, U>::_(u)); }
};

// T is a floating point type and U an integer type
template <typename T, typename U, typename C, typename E, Upscale D>
struct convert_sat_choice<T, U, C, E, IsFromInt, NotToInt, D> {
  // Assume that conversion to float from int does not saturate.
  static T _(const U &u) { return C::_(u); }
};

// U is floating point type, and T is a floating point type of larger precision
template <typename T, typename U, typename C, typename E>
struct convert_sat_choice<T, U, C, E, NotFromInt, NotToInt, IsUpscale> {
  static T _(const U &u) { return C::_(u); }
};

// U is floating point type, and T is a floating point type of the same or
// lower precision
template <typename T, typename U, typename C, typename E>
struct convert_sat_choice<T, U, C, E, NotFromInt, NotToInt, NotUpscale> {
  static T _(const U &u) {
    using TSigned = typename TypeTraits<T>::SignedType;
    using TElement = typename TypeTraits<T>::ElementType;

    T result = C::_(u);

    // Squash the value into the desired range.
    const T min_out = TypeTraits<TElement>::min();
    const U min_in = ConvertHelper<U, T, RTZHelper>::_(min_out);

    result = __abacus_select(
        result, min_out, abacus::detail::cast::convert<TSigned>(u < min_in));

    const T max_out = TypeTraits<TElement>::max();
    const U max_in = ConvertHelper<U, T, RTZHelper>::_(max_out);

    result = __abacus_select(
        result, max_out, abacus::detail::cast::convert<TSigned>(u > max_in));
    return result;
  }
};

// U is a floating point, T is an integer type
template <typename T, typename U, typename C, typename E, Upscale D>
struct convert_sat_choice<T, U, C, E, NotFromInt, IsToInt, D> {
  static T _(const U &u) {
    using TSigned = typename TypeTraits<T>::SignedType;
    using TElement = typename TypeTraits<T>::ElementType;

    T result = C::_(u);

    // Squash the value into the desired range.
    const T min_out = TypeTraits<TElement>::min();
    const U min_in = ConvertHelper<U, T, RTZHelper>::_(min_out);

    result = __abacus_select(
        result, min_out, abacus::detail::cast::convert<TSigned>(u < min_in));

    if (__abacus_isftz() && !TypeTraits<T>::is_signed) {
      // If we are in FTZ and targeting an unsigned type, we need to make sure
      // negative denormals properly get saturated to 0.
      result =
          __abacus_select(result, min_out,
                          abacus::detail::cast::convert<TSigned>(
                              ~__abacus_isnormal(u) & __abacus_signbit(u)));
    }

    const T max_out = TypeTraits<TElement>::max();
    const U max_in = ConvertHelper<U, T, RTZHelper>::_(max_out);

    result = __abacus_select(
        result, max_out, abacus::detail::cast::convert<TSigned>(u > max_in));

    // Saturate NAN to 0
    const TSigned nan_to_zero =
        abacus::detail::cast::convert<TSigned>(__abacus_isnan(u));
    result = __abacus_select(result, T(0), nan_to_zero);

    return result;
  }
};

#if defined(__CA_BUILTINS_HALF_SUPPORT)
// When converting half to cl_int and cl_long we can't represent the max
// and min values of these integer types in half precision(largest value
// +/- 65504), therefore we omit checks to saturate to these thresholds.
//
// The cases we do need to saturate are:
// * Converting from INFINITY.
// * Converting from a negative half value to an unsigned integer.
// * Converting from NaN. Section 6.2.3.3 of the spec states NaN should be
//   converted to 0 for integer destination types while in saturated mode.
template <typename T, typename U, typename C>
struct convert_sat_choice<T, U, C, abacus_half, NotFromInt, IsToInt,
                          IsUpscale> {
  static T _(const U &u) {
    using TSigned = typename TypeTraits<T>::SignedType;

    T result = C::_(u);

    // Saturate +/- INFINITY
    const TSigned sign_bit =
        abacus::detail::cast::convert<TSigned>(__abacus_signbit(u));

    const T max_out = TypeTraits<T>::max();
    const T min_out = TypeTraits<T>::min();
    const T inf_sat = __abacus_select(max_out, min_out, sign_bit);

    const TSigned is_inf =
        abacus::detail::cast::convert<TSigned>(__abacus_isinf(u));
    result = __abacus_select(result, inf_sat, is_inf);

    // Saturate NAN to 0
    const TSigned nan_to_zero =
        abacus::detail::cast::convert<TSigned>(__abacus_isnan(u));
    result = __abacus_select(result, T(0), nan_to_zero);

    // Saturate negative values to 0 when converting to an unsigned type
    if (!TypeTraits<T>::is_signed) {
      result = __abacus_select(result, T(0), sign_bit);
    }

    return result;
  }
};
#endif

// convert_*
template <typename T, typename U>
T convert(const U &u) {
  return DefaultConvertHelper<T, U>::_(u);
}

template <typename T, typename U>
T convert_rte(const U &u) {
  return ConvertHelper<T, U, RTEHelper>::_(u);
}

template <typename T, typename U>
T convert_rtn(const U &u) {
  return ConvertHelper<T, U, RTNHelper>::_(u);
}

template <typename T, typename U>
T convert_rtz(const U &u) {
  return ConvertHelper<T, U, RTZHelper>::_(u);
}

template <typename T, typename U>
T convert_rtp(const U &u) {
  return ConvertHelper<T, U, RTPHelper>::_(u);
}

// convert_sat*
template <typename T, typename U>
T convert_sat(const U &u) {
  return convert_sat_choice<T, U, DefaultConvertHelper<T, U>>::_(u);
}

template <typename T, typename U>
T convert_sat_rte(const U &u) {
  return convert_sat_choice<T, U, ConvertHelper<T, U, RTEHelper>>::_(u);
}

template <typename T, typename U>
T convert_sat_rtn(const U &u) {
  return convert_sat_choice<T, U, ConvertHelper<T, U, RTNHelper>>::_(u);
}

template <typename T, typename U>
T convert_sat_rtz(const U &u) {
  return convert_sat_choice<T, U, ConvertHelper<T, U, RTZHelper>>::_(u);
}

template <typename T, typename U>
T convert_sat_rtp(const U &u) {
  return convert_sat_choice<T, U, ConvertHelper<T, U, RTPHelper>>::_(u);
}
}  // namespace

#define DEF_WITH_BOTH_TYPES(IN_TYPE, OUT_TYPE)                        \
  abacus_##OUT_TYPE ABACUS_API __abacus_convert_##OUT_TYPE(           \
      abacus_##IN_TYPE x) {                                           \
    return convert<abacus_##OUT_TYPE>(x);                             \
  }                                                                   \
  abacus_##OUT_TYPE ABACUS_API __abacus_convert_##OUT_TYPE##_rte(     \
      abacus_##IN_TYPE x) {                                           \
    return convert_rte<abacus_##OUT_TYPE>(x);                         \
  }                                                                   \
  abacus_##OUT_TYPE ABACUS_API __abacus_convert_##OUT_TYPE##_rtn(     \
      abacus_##IN_TYPE x) {                                           \
    return convert_rtn<abacus_##OUT_TYPE>(x);                         \
  }                                                                   \
  abacus_##OUT_TYPE ABACUS_API __abacus_convert_##OUT_TYPE##_rtz(     \
      abacus_##IN_TYPE x) {                                           \
    return convert_rtz<abacus_##OUT_TYPE>(x);                         \
  }                                                                   \
  abacus_##OUT_TYPE ABACUS_API __abacus_convert_##OUT_TYPE##_rtp(     \
      abacus_##IN_TYPE x) {                                           \
    return convert_rtp<abacus_##OUT_TYPE>(x);                         \
  }                                                                   \
  abacus_##OUT_TYPE ABACUS_API __abacus_convert_##OUT_TYPE##_sat(     \
      abacus_##IN_TYPE x) {                                           \
    return convert_sat<abacus_##OUT_TYPE>(x);                         \
  }                                                                   \
  abacus_##OUT_TYPE ABACUS_API __abacus_convert_##OUT_TYPE##_sat_rte( \
      abacus_##IN_TYPE x) {                                           \
    return convert_sat_rte<abacus_##OUT_TYPE>(x);                     \
  }                                                                   \
  abacus_##OUT_TYPE ABACUS_API __abacus_convert_##OUT_TYPE##_sat_rtn( \
      abacus_##IN_TYPE x) {                                           \
    return convert_sat_rtn<abacus_##OUT_TYPE>(x);                     \
  }                                                                   \
  abacus_##OUT_TYPE ABACUS_API __abacus_convert_##OUT_TYPE##_sat_rtz( \
      abacus_##IN_TYPE x) {                                           \
    return convert_sat_rtz<abacus_##OUT_TYPE>(x);                     \
  }                                                                   \
  abacus_##OUT_TYPE ABACUS_API __abacus_convert_##OUT_TYPE##_sat_rtp( \
      abacus_##IN_TYPE x) {                                           \
    return convert_sat_rtp<abacus_##OUT_TYPE>(x);                     \
  }

#define DEF_INTERGRAL_TYPES(TYPE, SIZE)         \
  DEF_WITH_BOTH_TYPES(TYPE##SIZE, char##SIZE)   \
  DEF_WITH_BOTH_TYPES(TYPE##SIZE, short##SIZE)  \
  DEF_WITH_BOTH_TYPES(TYPE##SIZE, int##SIZE)    \
  DEF_WITH_BOTH_TYPES(TYPE##SIZE, long##SIZE)   \
  DEF_WITH_BOTH_TYPES(TYPE##SIZE, uchar##SIZE)  \
  DEF_WITH_BOTH_TYPES(TYPE##SIZE, ushort##SIZE) \
  DEF_WITH_BOTH_TYPES(TYPE##SIZE, uint##SIZE)   \
  DEF_WITH_BOTH_TYPES(TYPE##SIZE, ulong##SIZE)  \
  DEF_WITH_BOTH_TYPES(TYPE##SIZE, float##SIZE)

#if defined(__CA_BUILTINS_DOUBLE_SUPPORT) && defined(__CA_BUILTINS_HALF_SUPPORT)
#define DEF_WITH_TYPE_AND_SIZE(TYPE, SIZE)    \
  DEF_INTERGRAL_TYPES(TYPE, SIZE)             \
  DEF_WITH_BOTH_TYPES(TYPE##SIZE, half##SIZE) \
  DEF_WITH_BOTH_TYPES(TYPE##SIZE, double##SIZE)
#elif defined(__CA_BUILTINS_DOUBLE_SUPPORT)
#define DEF_WITH_TYPE_AND_SIZE(TYPE, SIZE) \
  DEF_INTERGRAL_TYPES(TYPE, SIZE)          \
  DEF_WITH_BOTH_TYPES(TYPE##SIZE, double##SIZE)
#elif defined(__CA_BUILTINS_HALF_SUPPORT)
#define DEF_WITH_TYPE_AND_SIZE(TYPE, SIZE) \
  DEF_INTERGRAL_TYPES(TYPE, SIZE)          \
  DEF_WITH_BOTH_TYPES(TYPE##SIZE, half##SIZE)
#else
#define DEF_WITH_TYPE_AND_SIZE(TYPE, SIZE) DEF_INTERGRAL_TYPES(TYPE, SIZE)
#endif

#define DEF(TYPE)                 \
  DEF_WITH_TYPE_AND_SIZE(TYPE, )  \
  DEF_WITH_TYPE_AND_SIZE(TYPE, 2) \
  DEF_WITH_TYPE_AND_SIZE(TYPE, 3) \
  DEF_WITH_TYPE_AND_SIZE(TYPE, 4) \
  DEF_WITH_TYPE_AND_SIZE(TYPE, 8) \
  DEF_WITH_TYPE_AND_SIZE(TYPE, 16)

#endif  //__ABACUS_CAST_CONVERT_HELPER_H__
