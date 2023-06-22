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
#include <abacus/abacus_integer.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/add_exact.h>
#include <abacus/internal/ldexp_unsafe.h>
#include <abacus/internal/multiply_exact.h>

namespace {

template <typename T>
struct fma_traits;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <>
struct fma_traits<abacus_half> {
  // 2^((Bits in Mantissa + 1) - Exponent Bias) = 2^(11 - 15)
  // Represented as a half
  static const abacus_ushort round_offset = 0x2C00;
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <>
struct fma_traits<abacus_float> {
  // 2^((Bits in Mantissa + 1) - Exponent Bias) = 2^(24 - 127)
  // Represented as a float
  static const abacus_uint round_offset = 0x0C000000;
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <>
struct fma_traits<abacus_double> {
  // 2^((Bits in Mantissa + 1) - Exponent Bias) = 2^(53 - 1023)
  // Represented as a double
  static const abacus_ulong round_offset = 0x0350000000000000;
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

// Works on all modes, but needs unsigned integer types of double the bit width
// of the floating point type. E.g. half uses uint and float uses ulong
template <typename T>
T ABACUS_API __abacus_fma_safe(const T x, const T y, const T z) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;
  typedef typename TypeTraits<UnsignedType>::LargerType ULType;
  typedef FPShape<T> Shape;

  const UnsignedType xUint = abacus::detail::cast::as<UnsignedType>(x);
  const UnsignedType yUint = abacus::detail::cast::as<UnsignedType>(y);
  const UnsignedType zUint = abacus::detail::cast::as<UnsignedType>(z);

  const UnsignedType sign_mask = Shape::SignMask();
  bool effective_add = (xUint ^ yUint ^ zUint) < sign_mask;

  if (__abacus_isnan(x) || __abacus_isnan(y) || __abacus_isnan(z)) {
    return FPShape<T>::NaN();
  }

  if (__abacus_isinf(x) || __abacus_isinf(y)) {
    return x * y + z;
  }

  if (__abacus_isinf(z) || x == T(0) || y == T(0)) {
    return z;
  }

  // extract the exponent and mantissa of each abacus_float:
  const SignedType exp_bias = Shape::Bias();
  const SignedType exp_mask = Shape::ExponentMask();
  const SignedType mantissa_bits = Shape::Mantissa();
  SignedType xExp = ((xUint & exp_mask) >> mantissa_bits) - exp_bias;
  SignedType yExp = ((yUint & exp_mask) >> mantissa_bits) - exp_bias;
  SignedType zExp = ((zUint & exp_mask) >> mantissa_bits) - exp_bias;

  const SignedType mant_mask = Shape::MantissaMask();
  const SignedType exp_lsb = Shape::LeastSignificantExponentBit();

  // Remembering the implicit mantissa bit
  UnsignedType xMant = (xExp == -exp_bias) ? ((xUint & mant_mask) << 1)
                                           : (xUint & mant_mask) | exp_lsb;
  UnsignedType yMant = (yExp == -exp_bias) ? ((yUint & mant_mask) << 1)
                                           : (yUint & mant_mask) | exp_lsb;
  UnsignedType zMant = (zExp == -exp_bias) ? ((zUint & mant_mask) << 1)
                                           : (zUint & mant_mask) | exp_lsb;

  const SignedType xyExp = xExp + yExp;
  SignedType expDiff = xyExp - zExp;

  // If the exponent of z is much larger than xy we won't have enough precision
  // to represent to addition of the small number and we can just return z
  if (expDiff < -SignedType(Shape::Mantissa()) - 3) {
    return z;
  }

  // Use larger type since multiplication will otherwise overflow now the
  // implicit 1.0 bit has been OR-ed into the mantissas.
  ULType xyMant = ULType(xMant) * ULType(yMant);
  ULType zLMant = zMant;

  // Shift the mantissas so that we can utilize the lower bits in future
  // operations to represent reduction in exponent.
  const SignedType num_bits = Shape::NumBits();
  zLMant <<= num_bits;

  // Subtract the number of mantissa bits from our shift to account for the
  // change in exponent during multiplication of mantissas.
  xyMant <<= num_bits - mantissa_bits;

  // Classify mantissas based on exponent
  ULType Mant_hi = (expDiff >= 0) ? xyMant : zLMant;
  ULType Mant_lo = (expDiff >= 0) ? zLMant : xyMant;
  SignedType higher_exponent = (expDiff >= 0) ? xyExp : zExp;

  T ansSign = (expDiff >= 0) ? x * y : z;

  expDiff = __abacus_abs(expDiff);

  // Shift Mant_lo down by expDiff and save the remainder in-case we need it
  // for rounding later.
  const SignedType UL_bits = sizeof(ULType) * 8;
  bool Sticky_Bit = (expDiff < UL_bits) ? (Mant_lo << (UL_bits - expDiff)) != 0
                                        : (Mant_lo != 0);
  if (expDiff == 0) {
    Sticky_Bit = 0;
  }

  // Shift the mantissa of the smaller value by the exponent difference
  Mant_lo = (expDiff < UL_bits) ? Mant_lo >> expDiff : 0;

  // If it's a subtraction negate the bits instead
  Mant_lo = (effective_add) ? Mant_lo : ~Mant_lo + 1;

  // Perform the addition
  ULType ansLMant = Mant_hi + Mant_lo;

  // In the subtraction case it will overflow if z > x*y but the exponents are
  // the same
  if (ansLMant > (ULType(0xF) << (UL_bits - 4))) {
    ansLMant = ~ansLMant + 1;
    ansSign = -ansSign;
    effective_add = !effective_add;
  }

  // Find out how many places we need to shift it down that the mantissa fits
  // inside the mantissa bits of the floating point type.
  abacus_int shift = 0;
  while ((ansLMant >> shift) > (mant_mask | exp_lsb)) {
    shift++;
  }

  // In the case of catastrophic cancellation, where x*y is very close to -z,
  // most of ansLMant will be 0's, and we'll need to up shift in the other
  // direction instead
  abacus_int cancellation_degree = 0;
  if ((ansLMant != 0) && exp_lsb > ansLMant) {
    // We need to instead shift up
    while (exp_lsb > ansLMant) {
      ansLMant <<= 1;
      cancellation_degree++;
    }
  }

  // Use shift to work out mantissa
  const UnsignedType ansMant = UnsignedType(ansLMant >> shift);

  // Record the remaining mantissa bits we don't have enough precision to keep
  UnsignedType ansMantRemainder =
      UnsignedType((shift < num_bits) ? ansLMant << (num_bits - shift)
                                      : ansLMant >> (shift - num_bits));

  // In extrememly rare cases, we might even shift off a bit off this resulting
  // in incorrect rounding.
  // This effects the answer if ansMantRemainder becomes exactly sign_mask,
  // but has had something rounded off at the bottom.
  // This can effect the final rounding.
  if (ansMantRemainder == sign_mask) {
    // Increase it by 1 to break the incorrect final rounding if it's required:
    if (shift > num_bits) {
      if ((ansLMant << (UL_bits - (shift - num_bits))) != 0) {
        ansMantRemainder++;
      }
    }
  }

  // Final exponent of the answer is the larger exponent of x*y and z, plus
  // an offset from the difference between the shift and number of float bits
  // we originally shifted the mantissas by.
  const SignedType ansExponent =
      higher_exponent + shift - num_bits - cancellation_degree;

  if (ansExponent > exp_bias) {
    return __abacus_copysign(T(ABACUS_INFINITY), ansSign);
  }

  if (ansMant == 0) {
    return T(0);
  }

  // construct the answer from ansMant and ansExponent:
  UnsignedType ansUint =
      (ansMant & mant_mask) | ((ansExponent + exp_bias) << mantissa_bits);

  // Denormal answer
  if (ansExponent <= -exp_bias) {
    const SignedType shift_amount =
        (exp_bias - 1) + higher_exponent - cancellation_degree;
    ansMantRemainder =
        UnsignedType((shift_amount >= 0) ? ansLMant << shift_amount
                                         : ansLMant >> -shift_amount);

    // Occasionally here we can cut off an important bit, especially if
    // ansMantRemainder ends up becoming sign_mask
    if ((ansMantRemainder == sign_mask) && shift_amount < 0) {
      // Increase it by 1 to break the incorrect final rounding if it's
      // required:
      if ((ansLMant << (UL_bits + shift_amount)) != 0) {
        ansMantRemainder++;
      }
    }

    ansUint = UnsignedType(ansLMant >> (num_bits - shift_amount));
  }

  // When to round up? Using Round to Nearest Even(rte) here
  bool round_up = 0;
  if (ansMantRemainder > sign_mask) {
    round_up = 1;
  }
  if ((ansMantRemainder == sign_mask) && (Sticky_Bit == 1)) {
    // If z is 0 here, effective add can change the answer when it shouldn't, if
    // the sign of the z is different to the sign of x*y.
    // If we got here it should always round if z == 0.0, so we add this to the
    // check:
    round_up = effective_add | ((zUint & Shape::InverseSignMask()) == 0);
  }
  if ((ansMantRemainder == sign_mask) && (Sticky_Bit == 0) &&
      ((ansUint & 0x1) == 1)) {
    round_up = 1;
  }

  ansUint += round_up;

  return __abacus_copysign(abacus::detail::cast::as<T>(ansUint), ansSign);
}

// Our __unsafe implementation works when using a round to negative inf(RTN)
// rounding mode, on an architecture which won't flush denormal numbers to zero
template <typename T>
T ABACUS_API __abacus_fma_unsafe(const T x, const T y, const T z) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;
  typedef FPShape<T> Shape;

  const UnsignedType sign_mask = Shape::SignMask();

  if (__abacus_isnan(x) || __abacus_isnan(y) || __abacus_isnan(z) ||
      __abacus_isinf(x) || __abacus_isinf(y)) {
    return x * y + z;
  }

  if (__abacus_isinf(z)) {
    return z;
  }

  const UnsignedType xUint = abacus::detail::cast::as<UnsignedType>(x);
  const UnsignedType yUint = abacus::detail::cast::as<UnsignedType>(y);
  const UnsignedType zUint = abacus::detail::cast::as<UnsignedType>(z);

  // Extract the exponents:
  const SignedType exp_bias = Shape::Bias();
  const SignedType exp_mask = Shape::ExponentMask();
  const SignedType mantissa_bits = Shape::Mantissa();

  SignedType xExp = ((xUint & exp_mask) >> mantissa_bits) - exp_bias;
  SignedType yExp = ((yUint & exp_mask) >> mantissa_bits) - exp_bias;
  SignedType zExp = ((zUint & exp_mask) >> mantissa_bits) - exp_bias;

  SignedType xyExp = xExp + yExp;

  // Get the larger of xyExp and zExp, to prepare scaling the calculation into a
  // higher range. We need to scale max(x*y, z) into as high a range as
  // possible, but still be sure that x*y + z won't overflow. The highest
  // exponent be can bring it into safely is EXPONENT_MAX - 3, as otherwise
  // adding x*y and z could potentially overflow
  const SignedType desired_range = Shape::Bias() - 3;
  SignedType ansExp = __abacus_max(xyExp, zExp) - desired_range;

  ansExp >>= 1;

  // We now bring the calculation into the range 2^(EXPONENT_MAX - 3) by using
  // an unsafe ldexp on the input values to bring them into a nicer range.
  // We do this to prevent intermediate denormal answers, by scaling
  // x, y, z to be as large as possible so that x*y + z never overflows
  const SignedType ldexp_factor_z = (-ansExp + exp_bias) << mantissa_bits;
  const T unsafe_ldexp_factor_z = abacus::detail::cast::as<T>(ldexp_factor_z);

  // In weird situations, like where x is very large and y is very small,
  // x*y exactly can mess up. We change this by bringing x and y into
  // roughly the same range by multipling x by a power of 2 and y
  // by the inverse power. This won't change x*y
  // The power we use is 2^((yExp - xExp) / 2) (The average exponent)

  // We also take this opportunity to scale the problem up to where we
  // want it to be, by also scaling by -ansExp. We can do all this in
  // one scaling:

  const SignedType xyExp_average = (yExp - xExp) / 2;

  // To support denormal inputs on ftz hardware use these two lines:
  // x = __abacus_ldexp(x, xyExp_average - ansExp);
  // y = __abacus_ldexp(y, -xyExp_average - ansExp);

  // Otherwise we can use the much faster ldexp_unsafe:
  const T x_ldexp = abacus::internal::ldexp_unsafe(x, xyExp_average - ansExp);
  const T y_ldexp = abacus::internal::ldexp_unsafe(y, -xyExp_average - ansExp);

  // Sometimes z can become 0 when we scale it here, so we save whether it was
  // or not:
  const T z_before_scaling = z;

  // To support a denormal z input on ftz hardware use this line:
  // T z_ldexp = __abacus_ldexp(z, 2*(-ansExp));

  // Otherwise this is much faster:
  T z_ldexp = (z * unsafe_ldexp_factor_z) * unsafe_ldexp_factor_z;

  // If z got underflowed to 0 in the above multiplication, it means
  // it doesn't matter to the calculation but it can make a difference
  // when the final rounding gets very close. So rather than deal with
  // that we just set z back to what it was, and it doesn't effect the
  // answer, as before, but does effect the rounding when it needs to.
  if (z_ldexp == T(0.0)) {
    z_ldexp = z_before_scaling;
  }

  // Exactly multiply x and y, save the answer in mul_hi and mul_lo
  T mul_lo{};
  T mul_hi = abacus::internal::multiply_exact(x_ldexp, y_ldexp, &mul_lo);

  // Exactly add in z, keeping the all the results in place:
  abacus::internal::add_exact_safe(&mul_hi, &z_ldexp);
  abacus::internal::add_exact_safe(&mul_lo, &z_ldexp);
  abacus::internal::add_exact(&mul_hi, &mul_lo);

  // Now we have the scaled answer in the form of the sum of 3 halfs: in
  // order of size mul_hi, mul_lo, and z, each an order of magnitude
  // above the other (aka (mul_hi + mul_lo) rounds to mul_hi,
  // and (mul_lo + z) rounds to mul_lo

  // We now need to ldexp back to the right scale, but we firstly see
  // if the combination of (mul_lo + z) could effect mul_hi's rounding
  // We also need some more work for denormal answers.

  // z will effect the rounding if mul_lo is exactly 0.5 ulps off mul_hi
  UnsignedType hiAsUint = abacus::detail::cast::as<UnsignedType>(mul_hi);
  UnsignedType loAsUint = abacus::detail::cast::as<UnsignedType>(mul_lo);
  UnsignedType zAsUint = abacus::detail::cast::as<UnsignedType>(z_ldexp);

  // 'round' is true if mul_lo is exactly between mul_hi and nextafter(mul_hi)
  // in either direction
  // (just checks if loAsUint is the correct power of 2 for this to happen)
  SignedType round =
      (hiAsUint & exp_mask) ==
      ((loAsUint & Shape::InverseSignMask()) + fma_traits<T>::round_offset);

  // If mul_hi is an exact power of 2 sometimes 'round' doesn't get
  // picked up on correctly, notably when mul_lo is an exact -0.5 ulp from
  // mul_hi, with mul_hi being an exact power of 2.
  // Check here for that:

  // Is mul_hi a power of 2?:
  if ((hiAsUint & Shape::MantissaMask()) == 0) {
    // Nice way to check that mul_lo is exactly -0.5 ulps from mul_hi
    // and with the opposite sign
    // Adding 0x8000 (sign_mask) checks the signs are the opposite,
    // and 0x3000 (round_offset_test) is the required exponent difference
    UnsignedType round_offset_test =
        fma_traits<T>::round_offset + (UnsignedType(0x1) << Shape::Mantissa());
    if (hiAsUint == UnsignedType(loAsUint + sign_mask + round_offset_test)) {
      round = true;
    }
  }

  // We now know if we need to round in a way that mightn't have been dealt
  // with, With (mul_hi + mul_lo) being exactly halfway between (mul_hi) and
  // nextafter(mul_hi, +-1), which has rounded to mul_hi. But z might effect
  // this in a way that hasen't been accounted for
  // Do that here:
  if (round && (z_ldexp != T(0.0)) &&
      // if mul_lo and z need to have the same sign:
      ((loAsUint & sign_mask) == (zAsUint & sign_mask))) {
    // z thus has an effect on mul_hi
    // Alter mul_hi in the direction of z;
    const SignedType rounding =
        ((hiAsUint & sign_mask) == (zAsUint & sign_mask)) ? 1 : -1;

    mul_hi = abacus::detail::cast::as<T>(SignedType(hiAsUint + rounding));

    // set "mul_lo" -> "-mul_lo" to balance out changing the last bit of mul_hi,
    // This makes sure (mul_hi + mul_lo) is mathematically the same as before
    mul_lo = -mul_lo;
    loAsUint = abacus::detail::cast::as<UnsignedType>(mul_lo);
  }

  // We now have a scaled answer that we need to scale down by 2^(2*ansExp)
  // We need to be careful of the case when this makes the final answer
  // denormal, as more rounding bits appear.
  T ans = mul_hi;
  const UnsignedType ansAsUint = abacus::detail::cast::as<UnsignedType>(ans);

  // If ldexp turns ans into a denorm it can round the wrong way as it doesn't
  // take mul_lo into account. This only happens if the bit ldexp cuts off is
  // an exact in between value, otherwise the rounding is ok.
  const SignedType finalExpBiased =
      (((ansAsUint & exp_mask) >> mantissa_bits)) + 2 * ansExp;

  if (finalExpBiased < 1) {
    // The answer is a denormal, find what bits will be cut off and see if it's
    // going to result in a RTE case.
    const UnsignedType rounding_bits = UnsignedType(0x1)
                                       << ((-finalExpBiased) + 1);
    const UnsignedType rounding_mask = rounding_bits - 1;

    const UnsignedType RTE_bit = UnsignedType(0x1) << (-finalExpBiased);
    if ((RTE_bit == (rounding_mask & ansAsUint)) ||
        // The above check doesn't catch the case of a minimum denormal,
        // when you end up rounding the hidden bit off.
        // The line below checks for that:
        (finalExpBiased == -10 && (ansAsUint & Shape::MantissaMask()) == 0)) {
      // So now we have a round to even going to happen when we ldexp at the
      // end. Now mul_lo will effect the answer. (if nonzero)
      // (side note: if mul_lo is 0.0 then z must also be 0.0 so we never need
      // to check for it)
      if (mul_lo != T(0.0)) {
        // We add/subtract one to the ushort of the answer. Prevents the RTE
        // situation when we scale and tips the rounding in the correct
        // direction for when we ldexp at the end for the final answer

        // If mul_lo is the opposite sign to mul_hi subtract 1 instead.
        const SignedType rounding =
            ((ansAsUint & sign_mask) == (loAsUint & sign_mask)) ? 1 : -1;
        ans = abacus::detail::cast::as<T>(SignedType(ansAsUint + rounding));
      }
    }
  }

  // To support denormal answers on ftz hardware use this:
  // return __abacus_ldexp(ans, 2 * int(ansExp));

  // Otherwise use this inlined ldexp:
  const T scaling_factor = abacus::detail::cast::as<T>(
      SignedType((ansExp + Shape::Bias()) << Shape::Mantissa()));
  ans = (ans * scaling_factor) * scaling_factor;

  return ans;
}

// Helper struct to select the adequate fma implementation
template <typename T, typename E = typename TypeTraits<T>::ElementType,
          bool SCALAR = 1 == TypeTraits<T>::num_elements>
struct fma_select {
  static T fma(const T x, const T y, const T z);
};

// Vector floats/halfs, use the safe implementation if we are in ftz mode.
template <typename T, typename E>
struct fma_select<T, E, false> {
  static T fma(const T x, const T y, const T z) {
    T r{};
    for (unsigned i = 0; i < TypeTraits<T>::num_elements; i++) {
      if (__abacus_isftz()) {
        r[i] = __abacus_fma_safe(x[i], y[i], z[i]);
      } else {
        r[i] = __abacus_fma_unsafe(x[i], y[i], z[i]);
      }
    }
    return r;
  }
};

// Scalar floats/halfs, use they safe implementation if we are in ftz mode.
template <typename T, typename E>
struct fma_select<T, E, true> {
  static T fma(const T x, const T y, const T z) {
    if (__abacus_isftz()) {
      return __abacus_fma_safe(x, y, z);
    } else {
      return __abacus_fma_unsafe(x, y, z);
    }
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
// Vector doubles
template <typename T>
struct fma_select<T, abacus_double, false> {
  static T fma(const T x, const T y, const T z) {
    T r{};
    for (unsigned i = 0; i < TypeTraits<T>::num_elements; i++) {
      r[i] = __abacus_fma_unsafe(x[i], y[i], z[i]);
    }
    return r;
  }
};

// Scalar doubles
template <typename T>
struct fma_select<T, abacus_double, true> {
  static T fma(const T x, const T y, const T z) {
    return __abacus_fma_unsafe(x, y, z);
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
inline T fma_helper(const T x, const T y, const T z) {
  return fma_select<T>::fma(x, y, z);
}
}  // namespace
#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_fma(abacus_half x, abacus_half y,
                                    abacus_half z) {
  return fma_helper(x, y, z);
}
abacus_half2 ABACUS_API __abacus_fma(abacus_half2 x, abacus_half2 y,
                                     abacus_half2 z) {
  return fma_helper(x, y, z);
}
abacus_half3 ABACUS_API __abacus_fma(abacus_half3 x, abacus_half3 y,
                                     abacus_half3 z) {
  return fma_helper(x, y, z);
}
abacus_half4 ABACUS_API __abacus_fma(abacus_half4 x, abacus_half4 y,
                                     abacus_half4 z) {
  return fma_helper(x, y, z);
}
abacus_half8 ABACUS_API __abacus_fma(abacus_half8 x, abacus_half8 y,
                                     abacus_half8 z) {
  return fma_helper(x, y, z);
}
abacus_half16 ABACUS_API __abacus_fma(abacus_half16 x, abacus_half16 y,
                                      abacus_half16 z) {
  return fma_helper(x, y, z);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_fma(abacus_float x, abacus_float y,
                                     abacus_float z) {
  return fma_helper(x, y, z);
}
abacus_float2 ABACUS_API __abacus_fma(abacus_float2 x, abacus_float2 y,
                                      abacus_float2 z) {
  return fma_helper(x, y, z);
}
abacus_float3 ABACUS_API __abacus_fma(abacus_float3 x, abacus_float3 y,
                                      abacus_float3 z) {
  return fma_helper(x, y, z);
}
abacus_float4 ABACUS_API __abacus_fma(abacus_float4 x, abacus_float4 y,
                                      abacus_float4 z) {
  return fma_helper(x, y, z);
}
abacus_float8 ABACUS_API __abacus_fma(abacus_float8 x, abacus_float8 y,
                                      abacus_float8 z) {
  return fma_helper(x, y, z);
}
abacus_float16 ABACUS_API __abacus_fma(abacus_float16 x, abacus_float16 y,
                                       abacus_float16 z) {
  return fma_helper(x, y, z);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_fma(abacus_double x, abacus_double y,
                                      abacus_double z) {
  return fma_helper(x, y, z);
}
abacus_double2 ABACUS_API __abacus_fma(abacus_double2 x, abacus_double2 y,
                                       abacus_double2 z) {
  return fma_helper(x, y, z);
}
abacus_double3 ABACUS_API __abacus_fma(abacus_double3 x, abacus_double3 y,
                                       abacus_double3 z) {
  return fma_helper(x, y, z);
}
abacus_double4 ABACUS_API __abacus_fma(abacus_double4 x, abacus_double4 y,
                                       abacus_double4 z) {
  return fma_helper(x, y, z);
}
abacus_double8 ABACUS_API __abacus_fma(abacus_double8 x, abacus_double8 y,
                                       abacus_double8 z) {
  return fma_helper(x, y, z);
}
abacus_double16 ABACUS_API __abacus_fma(abacus_double16 x, abacus_double16 y,
                                        abacus_double16 z) {
  return fma_helper(x, y, z);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
