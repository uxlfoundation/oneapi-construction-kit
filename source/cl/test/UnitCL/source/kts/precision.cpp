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

#include "kts/precision.h"

namespace {
/// @brief Performs RTE rounding on the mantissa bits not available in half
///        RTE rounding is Round To nearest, ties to Even.
///
/// @param x Mantissa bits of a single or double precision floating point number
///
/// @return rounded mantissa, or max unsigned type in the case of overflow
template <class T, class U = typename kts::ucl::TypeInfo<T>::AsUnsigned>
U RTEMantissa(U x) {
  static_assert(std::is_floating_point<T>::value,
                "Function only applicable to cl_float and cl_double");
  using unsigned_t = typename kts::ucl::TypeInfo<T>::AsUnsigned;
  const unsigned mantissa_bit_diff = kts::ucl::TypeInfo<T>::mantissa_bits -
                                     kts::ucl::TypeInfo<cl_half>::mantissa_bits;

  // Most significant bit lost in mantissa rounding
  const unsigned_t lead_bit = unsigned_t(1) << (mantissa_bit_diff - 1);
  // Mask for the rest of the lost mantissa bits
  const unsigned_t bottom_bits = lead_bit - 1;

  // In RTE rounding we round up if the most significant bit is set, and
  // at least one other bit.
  unsigned_t out = 0u;
  if (0 == (x & lead_bit)) {
    // Nearest next number is towards zero if first excess bit is zero.
    // Mask away all the extra mantissa bits
    out = x & ~(bottom_bits | lead_bit);
  } else if (0 != (x & bottom_bits)) {
    // Nearest is away from zero if first excess bit is 1, and
    // so is at least one other excess bit from the rest of the mantissa.
    const unsigned_t increment = (x >> mantissa_bit_diff) + 1;
    out = increment << mantissa_bit_diff;
  } else {
    // First excess bit is set, but none of the remaining dropped mantissa bits
    // are. This means that there is no nearest rounding so we need to
    // break the tie by rounding towards even. Binary numbers are even if
    // they end in 0, which in this case is the last bit of the half mantissa.
    if ((x & (unsigned_t(1) << mantissa_bit_diff)) == 0) {
      // Even, set all excess bits to zero
      out = x & ~(bottom_bits | lead_bit);
    } else {
      // Odd, increment away from zero
      const unsigned_t increment = (x >> mantissa_bit_diff) + 1;
      out = increment << mantissa_bit_diff;
    }
  }
  if (out > kts::ucl::TypeInfo<T>::mantissa_mask) {
    // Our mantissa has overflowed while rounding away from zero,
    // return unsigned max to make this more obvious to caller.
    return std::numeric_limits<unsigned_t>::max();
  }
  return out;
}

/// @brief Performs RTP, RTN, or RTZ rounding on the mantissa bits not available
///        in half preicions.
///
/// RTP rounding is Round To Positive infinity, RTN is Negative infinity, RTZ
/// is Round to Zero.
///
/// @param x Mantissa bits of a single or double precision floating point number
/// @param to_zero Round towards zero if set, away from zero otherwise
///
/// @return rounded mantissa, or max unsigned type in the case of overflow
template <class T, class U = typename kts::ucl::TypeInfo<T>::AsUnsigned>
U RTZeroMantissa(U x, bool to_zero) {
  static_assert(std::is_floating_point<T>::value,
                "Function only applicable to cl_float and cl_double");
  using unsigned_t = typename kts::ucl::TypeInfo<T>::AsUnsigned;
  const unsigned mantissa_bit_diff = kts::ucl::TypeInfo<T>::mantissa_bits -
                                     kts::ucl::TypeInfo<cl_half>::mantissa_bits;

  // Bits which will be rounded off
  const unsigned_t dropped_bits = (unsigned_t(1) << mantissa_bit_diff) - 1;
  if (0 == (x & dropped_bits)) {
    return x;
  }

  if (to_zero) {
    return x & ~dropped_bits;
  }

  const unsigned_t increment = (x >> mantissa_bit_diff) + 1;
  const unsigned_t out = increment << mantissa_bit_diff;

  if (out > kts::ucl::TypeInfo<T>::mantissa_mask) {
    // Our mantissa has overflowed while rounding away from zero,
    // return unsigned max to make this more obvious to caller.
    return std::numeric_limits<unsigned_t>::max();
  }
  return out;
}
}  // end namespace

namespace kts {
namespace ucl {

bool IsNaN(cl_half x) {
  // NaN is all exponent bits set and at one or more mantissa bits
  const cl_ushort abs = x & ~TypeInfo<cl_half>::sign_bit;
  const cl_ushort exp_mask = TypeInfo<cl_half>::exponent_mask;
  const cl_ushort mantissa_mask = TypeInfo<cl_half>::mantissa_mask;

  bool is_nan = (abs & mantissa_mask) && ((abs & exp_mask) == exp_mask);
  return is_nan;
}

bool IsInf(cl_half x) {
  // +Inf/-Inf is all exponent bits set and no mantissa bits
  const cl_ushort abs = x & ~TypeInfo<cl_half>::sign_bit;
  const cl_ushort exp_mask = TypeInfo<cl_half>::exponent_mask;
  return abs == exp_mask;
}

bool IsFinite(cl_half x) {
  // Finite values can't have all exponent bits set
  const cl_ushort abs = x & ~TypeInfo<cl_half>::sign_bit;
  const cl_ushort exp_mask = TypeInfo<cl_half>::exponent_mask;
  return abs < exp_mask;
}

bool IsNormal(cl_half x) {
  // Normal values must have one or more exponent bits set, but not all
  const cl_ushort abs = x & ~TypeInfo<cl_half>::sign_bit;
  const cl_ushort exp_mask = TypeInfo<cl_half>::exponent_mask;
  const cl_ushort mantissa_mask = TypeInfo<cl_half>::mantissa_mask;
  return (abs < exp_mask) & (abs > mantissa_mask);
}

cl_float ConvertHalfToFloat(const cl_half x) {
  cl_uint fp32 = (x & TypeInfo<cl_half>::sign_bit) << 16;
  const bool isDenormal = (x & TypeInfo<cl_half>::exponent_mask) == 0 &&
                          (x & TypeInfo<cl_half>::mantissa_mask) != 0;
  if (isDenormal) {
    // The input is a denormal number, scale it up.  Keep on doubling the
    // mantissa and lowering the exponent by 1, until the mantissa does not
    // fit into the 10-bits provided for 16-bit float's mantissas, then take
    // the 10 bits left in the Mantissa as the new mantissa (padded with 13
    // or 42 more zero bits for float), and the produced exponent as the new
    // exponent (with bias factors applied).  This works because denormal
    // numbers are a fixed-point representation, i.e. linearly spaced, so
    // doubling mantissa and subtracting one from exponent is a mathematical
    // no-op.
    // Note: lowering the exponent is done via addition in the loop, because
    // it is subtracted at the end.
    cl_int exponent = -1;  // No bias taken into account
    cl_ushort mantissa = x & TypeInfo<cl_half>::mantissa_mask;
    do {
      exponent++;
      mantissa <<= 1u;
    } while ((mantissa & (1 << TypeInfo<cl_half>::mantissa_bits)) == 0);

    fp32 |= (mantissa & TypeInfo<cl_half>::mantissa_mask) << 13;

    // It is worth remembering here that the implicit 1.0 in normal numbers was
    // not present in the denormal mantissa. So a denormal half with only the
    // first mantissa bit set is 0.5 * 2^-14, but this will be represented
    // in single precision as a 2^-15 float with no mantissa bits because of
    // implicit 1.0.
    exponent = TypeInfo<cl_float>::bias - TypeInfo<cl_half>::bias - exponent;
    fp32 |= exponent << TypeInfo<cl_float>::mantissa_bits;
  } else if (!IsFinite(x)) {
    // The input is an inf or a NaN (it doesn't really matter which).  Set
    // all exponent bits of output to 1, and preserve sign and mantissa (with
    // scaling).
    fp32 |= (x & TypeInfo<cl_half>::mantissa_mask) << 13;
    fp32 |= TypeInfo<cl_float>::exponent_mask;
  } else if ((x & 0x7FFF) != 0) {
    // This is just a normal non-zero number, scale appropriately.
    // Mantissa: copy, but with 13 extra zero bits in float.
    // Exponent: copy, but adjust for the difference in bias.
    fp32 |= (x & TypeInfo<cl_half>::mantissa_mask) << 13;

    cl_uint exponent = (x & TypeInfo<cl_half>::exponent_mask) >>
                       TypeInfo<cl_half>::mantissa_bits;

    exponent = TypeInfo<cl_float>::bias - TypeInfo<cl_half>::bias + exponent;
    fp32 |= exponent << TypeInfo<cl_float>::mantissa_bits;
  }
  return cargo::bit_cast<cl_float>(fp32);
}

template <class T>
cl_half ConvertFloatToHalf(const T x, RoundingMode rounding) {
  static_assert(std::is_floating_point<T>::value,
                "Function only applicable to cl_float and cl_double");
  // Initialize return value to signed zero
  using unsigned_t = typename TypeInfo<T>::AsUnsigned;
  unsigned_t as_unsigned = cargo::bit_cast<unsigned_t>(x);
  const unsigned size_diff = (sizeof(T) - sizeof(cl_half)) * 8;
  cl_half fp16 = (as_unsigned >> size_diff) & TypeInfo<cl_half>::sign_bit;

  // If input was zero just return zero with appropriate sign straight away
  if (T(0.0) == x) {
    return fp16;
  }

  int exponent = std::ilogb(x);  // Unbiased exponent
  unsigned_t mantissa = as_unsigned & TypeInfo<T>::mantissa_mask;
  const unsigned lost_mantissa_bits =
      TypeInfo<T>::mantissa_bits - TypeInfo<cl_half>::mantissa_bits;

  if (FP_ILOGBNAN == exponent || std::numeric_limits<int>::max() == exponent) {
    // Inf or Nan case
    fp16 |= TypeInfo<cl_half>::exponent_mask;
    if (mantissa) {
      fp16 |= 0x1;  // Can set any mantissa bit for NaN
    }
  } else if (exponent > TypeInfo<cl_half>::bias) {
    // Exponents greater than 15 cannot be represented in half.
    const cl_ushort infinity = TypeInfo<cl_half>::exponent_mask;
    const cl_ushort max_value = TypeInfo<cl_half>::max_float_bits;
    switch (rounding) {
      case RoundingMode::NONE:
      case RoundingMode::RTE:
        fp16 |= infinity;
        break;
      case RoundingMode::RTZ:
        fp16 |= max_value;
        break;
      case RoundingMode::RTP:
        fp16 |= std::signbit(x) ? max_value : infinity;
        break;
      case RoundingMode::RTN:
        fp16 |= std::signbit(x) ? infinity : max_value;
        break;
    }
  } else if (exponent < -14) {
    // Exponents less than -14 are represented using denormal numbers in half,
    // this means no exponent bits are set. Denormals halfs have an exponent
    // of -14 but no implicit 1.0 in the mantissa.

    // Take 10 highest order bits of the mantissa, drop bits not available in
    // half
    cl_ushort m =
        (as_unsigned >> lost_mantissa_bits) & TypeInfo<cl_half>::mantissa_mask;

    // Sets an extra MSB bit representing the implicit `1.0` that's present in
    // all normal float numbers, but absent in denormals. We now have 11 bits
    // of mantissa
    m |= 0x0400u;

    // The smaller our exponent the more we need to right shift the mantissa
    // to bring down the overall magnitude.
    const unsigned shift = -14 - exponent;
    const unsigned lost_bits = shift + lost_mantissa_bits;

    if (lost_bits > (TypeInfo<T>::mantissa_bits + 1)) {
      // We've shifted away all the bits
      // Number is too small to be representable by half
      switch (rounding) {
        case RoundingMode::NONE:
        case RoundingMode::RTE:
        case RoundingMode::RTZ:
          // return signed zero
          return fp16;
        case RoundingMode::RTP:
          // round up to -0 or smallest positive denormal
          return std::signbit(x) ? 0x8000 : 0x0001;
        case RoundingMode::RTN:
          // round down to +0 or smallest negative denormal
          return std::signbit(x) ? 0x8001 : 0x0000;
      }
    }

    const cl_ushort shifted_m = m >> shift;
    fp16 |= shifted_m;

    // Perform rounding based on all the mantissa bits we've dropped so far
    const unsigned_t rounding_mask = (unsigned_t(1) << lost_bits) - 1;
    const unsigned_t rounding_bits = as_unsigned & rounding_mask;

    // We only check for the rounding cases where we round away from zero and
    // increment the mantissa
    if (RoundingMode::RTN == rounding || RoundingMode::RTP == rounding) {
      const bool should_increment =
          rounding == RoundingMode::RTN ? std::signbit(x) : !std::signbit(x);
      if (should_increment && (0 != rounding_bits)) {
        // Round To Positive/Negative infinity round up depending on if the sign
        // bit isn't/is set respectively
        fp16 += 1;
      }
    } else if (RoundingMode::NONE == rounding ||
               RoundingMode::RTE == rounding) {
      const unsigned_t most_significant_bit = unsigned_t(1) << (lost_bits - 1);
      if ((rounding_bits & most_significant_bit) != 0) {
        // If we reach here most significant dropped mantissa bit is non-zero
        if ((rounding_bits & (most_significant_bit - 1)) != 0) {
          // As well as most significant bit being set, there is at least one
          // other. Round away from zero.

          // Overflow because of rounding is okay because it will turn fp16 from
          // 0x3FF (0.9998779296875 * 2^-14) into 0x400 (1.0 * 2^-14) because of
          // the implicit 1.0 in the mantissa once the exponent is normalized.
          fp16 += 1;
        } else if ((fp16 & 0x1) != 0) {
          // All other dropped mantissa bits after most significant are zero
          // Break round to nearest tie by rounding to even. A binary number is
          // even when it's last bit is zero.
          fp16 += 1;
        }
      }

      // A non-zero number has been interpreted as zero
      if (rounding_bits != 0 && (fp16 == 0 || fp16 == 0x8000)) {
        fp16++;
      }
    }
  } else {
    // Normal number we can represent
    exponent += TypeInfo<cl_half>::bias;  // Take into account the half exponent
                                          // bias of 15

    // Round away the last 13-bits of mantissa since this precision is not
    // available in half.
    unsigned_t rounded_mantissa = 0;
    switch (rounding) {
      case RoundingMode::NONE:
      case RoundingMode::RTE:
        rounded_mantissa = RTEMantissa<T>(mantissa);
        break;
      case RoundingMode::RTZ:
        rounded_mantissa =
            RTZeroMantissa<T>(mantissa, true /* always round to zero */);
        break;
      case RoundingMode::RTP:
        rounded_mantissa = RTZeroMantissa<T>(mantissa, std::signbit(x));
        break;
      case RoundingMode::RTN:
        rounded_mantissa = RTZeroMantissa<T>(mantissa, !std::signbit(x));
        break;
    }

    if (std::numeric_limits<unsigned_t>::max() == rounded_mantissa) {
      // Mantissa has overflowed as a result of rounding.
      exponent++;
      mantissa = 0;
    } else {
      // Floats have 13 more mantissa bits than half, so shift them away now
      // rounding has taken them into account.
      mantissa = rounded_mantissa >> lost_mantissa_bits;
    }

    fp16 |= exponent << TypeInfo<cl_half>::mantissa_bits;
    fp16 |= mantissa;
  }
  return fp16;
}

// Only valid template instantiations are float and double, declare here to
// avoid linking errors.
template cl_half ConvertFloatToHalf<cl_float>(const cl_float, RoundingMode);
template cl_half ConvertFloatToHalf<cl_double>(const cl_double, RoundingMode);

cl_float calcHalfPrecisionULP(const cl_float reference, const cl_half test) {
  cl_float test_as_float = ConvertHalfToFloat(test);
  const cl_half ref_as_half = ConvertFloatToHalf(reference);

  if (ref_as_half == test) {
    // catches reference overflow and underflow in half range
    return 0.0f;
  }

  if (std::isnan(reference) && std::isnan(test_as_float)) {
    // NaNs don't need to be bit exact
    return 0.0f;
  }

  if (std::isinf(reference)) {
    if (test_as_float == reference) {
      return 0.0f;
    }

    return test_as_float - reference;
  }

  // Check for overflow within allowable ULP range
  // https://github.com/KhronosGroup/OpenCL-CTS/pull/600
  if (std::isinf(test_as_float)) {
    // 2**16 is next representable value on the half number line
    test_as_float = std::copysign(65536.0f, test_as_float);
  }

  int reference_exp = std::ilogb(reference);
  if (0 == (cargo::bit_cast<uint32_t>(reference) &
            TypeInfo<cl_float>::mantissa_mask)) {
    // Reference is power of two
    reference_exp--;
  }

  constexpr int min_exp_bound = TypeInfo<cl_half>::min_exp - 1;
  const int ulp_exp =
      TypeInfo<cl_half>::mantissa_bits - std::max(reference_exp, min_exp_bound);

  // Scale the absolute error by the exponent
  return std::scalbn(test_as_float - reference, ulp_exp);
}

cl_double calcHalfPrecisionULP(const cl_double reference, const cl_half test) {
  cl_double test_as_float = ConvertHalfToFloat(test);
  const cl_half ref_as_half = ConvertFloatToHalf(reference);

  if (ref_as_half == test) {
    // catches reference overflow and underflow in half range
    return 0.0f;
  }

  if (std::isnan(reference) && std::isnan(test_as_float)) {
    // NaNs don't need to be bit exact
    return 0.0f;
  }

  if (std::isinf(reference)) {
    if (test_as_float == reference) {
      return 0.0f;
    }

    return test_as_float - reference;
  }

  // Check for overflow within allowable ULP range
  // https://github.com/KhronosGroup/OpenCL-CTS/pull/600
  if (std::isinf(test_as_float)) {
    // 2**16 is next representable value on the half number line
    test_as_float = std::copysign(65536.0f, test_as_float);
  }

  int reference_exp = std::ilogb(reference);
  if (0 == (cargo::bit_cast<uint64_t>(reference) &
            TypeInfo<cl_double>::mantissa_mask)) {
    // Reference is power of two
    reference_exp--;
  }

  constexpr int min_exp_bound = TypeInfo<cl_half>::min_exp - 1;
  const int ulp_exp =
      TypeInfo<cl_half>::mantissa_bits - std::max(reference_exp, min_exp_bound);

  // Scale the absolute error by the exponent
  return std::scalbn(test_as_float - reference, ulp_exp);
}

bool IsDenormalAsHalf(cl_float x) {
  // Minimum normal value representable by half is 2^-14
  return std::fabs(x) < 0.00006103515625f;
}

void InputGenerator::DumpSeed() const {
  std::cout << "Random numbers generated using std::mt19937 with seed " << seed_
            << "\n";
}

const std::array<cl_ushort, 26> InputGenerator::half_edge_cases = {{
    TypeInfo<cl_half>::exponent_mask | 0x1,
    TypeInfo<cl_half>::exponent_mask | TypeInfo<cl_half>::sign_bit | 0x1,
    TypeInfo<cl_half>::exponent_mask,
    TypeInfo<cl_half>::exponent_mask | TypeInfo<cl_half>::sign_bit,
    0,
    TypeInfo<cl_half>::sign_bit,
    0x3C00 /* 1.0 */,
    0xBC00 /* -1.0 */,
    0x4000 /* 2.0 */,
    0xC000 /* -2.0 */,
    0x1,
    0x1 | TypeInfo<cl_half>::sign_bit,
    0x2,
    0x2 | TypeInfo<cl_half>::sign_bit,
    TypeInfo<cl_half>::mantissa_mask,
    TypeInfo<cl_half>::mantissa_mask | TypeInfo<cl_half>::sign_bit,
    0x07FF,
    0x07FF | TypeInfo<cl_half>::sign_bit,
    0x0400,
    0x0400 | TypeInfo<cl_half>::sign_bit,
    0x01A3,
    0x01A3 | TypeInfo<cl_half>::sign_bit,
    0x02C9,
    0x02C9 | TypeInfo<cl_half>::sign_bit,
    0x7BFF,
    0x7BFF | TypeInfo<cl_half>::sign_bit,
}};

void InputGenerator::GenerateFloatData(std::vector<cl_half> &buffer) {
  // Generate half types by bitcasting ushorts. In the future
  // we should refine this to normal/denormal values. Or check
  // if all the half types can fit in the buffer.
  const size_t num_half_variants = 65536u;  // 2^16
  const size_t N = buffer.size();
  typedef typename TypeInfo<cl_half>::AsUnsigned UInt;

  if (N >= num_half_variants) {
    // We have enough space to represent the full range of half
    // precision, so make sure we do.
    for (size_t i = 0; i < N; i++) {
      UInt asUnsigned = i % num_half_variants;
      buffer[i] = cargo::bit_cast<cl_half>(asUnsigned);
    }
  } else {
    // We don't have enough buffer space to represent all half
    // values, so pick some using uniform distribution.
    const cl_ushort min = std::numeric_limits<cl_ushort>::min();
    const cl_ushort max = std::numeric_limits<cl_ushort>::max();
    std::uniform_int_distribution<cl_ushort> dist(min, max);
    std::generate(buffer.begin(), buffer.end(), [&] { return dist(gen_); });
  }

  // Shuffle vector so edge cases aren't always at the start
  std::shuffle(buffer.begin(), buffer.end(), gen_);
}

}  // namespace ucl
}  // namespace kts
