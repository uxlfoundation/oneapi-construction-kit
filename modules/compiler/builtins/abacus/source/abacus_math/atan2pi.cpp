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
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/add_exact.h>
#include <abacus/internal/horner_polynomial.h>

namespace {
template <typename T>
T atan2pi(const T y, const T x) {
  return __abacus_atan2(y, x) * T(ABACUS_1_PI);
}

#ifdef __CA_BUILTINS_HALF_SUPPORT
// atan2pi polynomial over range [1e-24, 1.2], see atan2pi sollya script
//
// Note that the first number is single precision, which is split into two FP16
// numbers: 3.18115234375e-1f16 and 1.5985965728759765625e-4f16
static ABACUS_CONSTANT abacus_half _atan2piH[5] = {
    3.18115234375e-1f16, -0.10479736328125f16, 5.5084228515625e-2f16,
    -2.32391357421875e-2f16, 4.69970703125e-3f16};

// atan2pi polynomial over range [1e-24, 0.186279] for devices where FTZ is
// enabled, see atan2pi sollya script.
//
// Notes:
// * These constants are multiplied by 8, to avoid FTZ conditions. The result is
// then scaled by 1/8 to negate this effect.
// * The first number is single precision, which can be split into two FP16
// numbers: 0.318359375f16 and -4.491209e-5f16
static ABACUS_CONSTANT abacus_half _atan2piH_ftz[3] = {
    -0.00035929672f16,                   // -4.491209e-05 x 8
    -3.6907196044921875e-4f16 * 8.0f16,  // -2.9526e-3
    -0.10223388671875f16 * 8.0f16,       // -0.81787
};

template <typename T>
T atan2pi_horner_polynomial(const T x2) {
  // In this particular builtin, we lose the most precision in the first
  // multiply-add in the horner polynomial. Replacing the first addition with
  // `add_exact`, then computing the rest of the horner polynomial by hand gives
  // us some extra precision.
  T first_iter_lo;
  const T first_iter_hi = abacus::internal::add_exact<T>(
      _atan2piH[3], x2 * _atan2piH[4], &first_iter_lo);

  T poly = _atan2piH[2] + ((x2 * first_iter_lo) + (x2 * first_iter_hi));
  poly = _atan2piH[1] + (x2 * poly);
  poly = _atan2piH[0] + (x2 * poly);
  return poly;
}

template <typename T>
T atan2pi_half(const T y, const T x) {
  using UnsignedType = typename TypeTraits<T>::UnsignedType;
  using SignedType = typename TypeTraits<T>::SignedType;
  using Shape = FPShape<abacus_half>;

  const UnsignedType sign_mask = Shape::SignMask();

  const SignedType inverse = __abacus_fabs(y) >= T(1.2f16 * __abacus_fabs(x));

  const T ratio = __abacus_select((y / x), (x / y), inverse);

  const T x2 = ratio * ratio;

  const T poly = atan2pi_horner_polynomial(x2);

  // Adding this small constant to the polynomial, then multiplying by 'ratio',
  // loses too much precision in cases where the other operands are large.
  // e.g, `T ans = (extra_precision + poly) * ratio;'
  // Using 2 separate multiplies added together instead preserves accuracy
  // here.
  const T extra_precision = 1.5985965728759765625e-4f16;
  T ans;
  if (__abacus_isftz()) {
    // We need to scale this calculation by 32 to avoid FTZ on some hardware.
    const T scaled_ratio = ratio * 32.0f16;
    ans = (extra_precision * scaled_ratio) + (poly * scaled_ratio);
    ans *= 0.03125f16;
  } else {
    ans = (extra_precision * ratio) + (poly * ratio);
  }

  if (__abacus_isftz()) {
    const T abs_ratio = __abacus_fabs(ratio);

    // x * (0.3183144629001617431640625 + x * (-3.6907196044921875e-4 + x *
    // (-0.10223388671875)))
    //
    // The first number is single precision, which can be split into two FP16
    // numbers:
    //  0.318359375f16 and -4.491209e-5f16
    //
    // To avoid flushing to zero, we multiply the ratio by 8, then later
    // divide by 8. This requires us to pre-multiply the polynomial constants
    // by 8.

    // 0.318359375f16 x 8
    const T remaining_term = 0.318359375f16 * 8.0f16;

    // Compute horner polynomial.
    T ans_ftz = abacus::internal::horner_polynomial(abs_ratio, _atan2piH_ftz);
    ans_ftz = remaining_term + ans_ftz;
    // Perform final multiplcation, then undo the scaling.
    ans_ftz = (abs_ratio * ans_ftz) * 0.125f16;
    // As atan(-ratio) = -atan(ratio), we need to copy the sign of ratio.
    ans_ftz = __abacus_copysign(ans_ftz, ratio);

    ans = __abacus_select(ans, ans_ftz, SignedType(abs_ratio <= 0.186279f16));

    // There is a special case failure where x2 == 0.7646484375. In this case,
    // we need to set ans to +/-0.228759765625 (depending on the sign of ratio).
    ans = __abacus_select(
        ans,
        __abacus_copysign(abacus::detail::cast::as<T>(UnsignedType(0x3352)),
                          ratio),
        abacus::detail::cast::as<UnsignedType>(x2) == UnsignedType(0x3A1E));
  }

  T pi_multiplication_factor = 0.0f16;

  pi_multiplication_factor = __abacus_select(
      pi_multiplication_factor, __abacus_copysign(0.5f16, ans), inverse);
  ans = __abacus_select(ans, -ans, inverse);

  T pi_multiplication_factor_negative_x =
      __abacus_select(T(-1.0f16), T(1.0f16),
                      abacus::detail::cast::as<UnsignedType>(y) < sign_mask);

  pi_multiplication_factor = __abacus_select(
      pi_multiplication_factor,
      pi_multiplication_factor + pi_multiplication_factor_negative_x,
      x < 0.0f16);

  // single value fix:
  T fix_value = abacus::detail::cast::as<T>(
      abacus::detail::cast::as<UnsignedType>(ans) - UnsignedType(1));

  // Weird but consistent bit pattern fix, it relies on the answer as opposed to
  // the inputs. It seems to cause an error because the double reference answer
  // is just below a power of 2, such that when rounded to a half it becomes a
  // power of 2. This has the effect of doubling the perceived ULP error. The
  // power of 2 is always the same:
  //   0.125 = 2^-3 = 0x3001
  //   3.125e-2 = 2^-5 = 0x2801
  ans = __abacus_select(
      ans, fix_value,
      (abacus::detail::cast::as<UnsignedType>(__abacus_fabs(ans)) == 0x3001) ||
          (abacus::detail::cast::as<UnsignedType>(__abacus_fabs(ans)) ==
           0x2801));
  ans = ans + pi_multiplication_factor;

  ans = __abacus_select(ans, __abacus_copysign(0.5f16, y), x == 0.0f16);

  T infinity_ans = __abacus_select(T(0.75f16), T(0.25f16), (x > 0.0f16));
  infinity_ans = __abacus_copysign(infinity_ans, y);
  ans = __abacus_select(ans, infinity_ans,
                        __abacus_isinf(x) && __abacus_isinf(y));

  ans = __abacus_select(ans, y, __abacus_isnan(y));

  // If x == y == 0.0, atan2pi() has the following edge cases defined by OpenCL
  // atan2pi(+/-0, -0) = +/-1.0
  // atan2pi(+/-0, +0) = +/-0.0
  const T zeros_ans = __abacus_copysign(
      __abacus_select(T(0.0f16), T(1.0f16), __abacus_signbit(x)), y);
  ans = __abacus_select(ans, zeros_ans, (x == 0.0f16) && (y == 0.0f16));

  return ans;
}

template <>
abacus_half atan2pi_half(const abacus_half y, const abacus_half x) {
  using Shape = FPShape<abacus_half>;

  const abacus_ushort sign_mask = Shape::SignMask();

  // If x == y == 0.0, atan2pi() has the following edge cases defined by OpenCL
  if ((x == 0.0f16) && (y == 0.0f16)) {
    // atan2pi(+/-0, -0) = +/-1.0
    // atan2pi(+/-0, +0) = +/-0.0
    return __abacus_copysign(__abacus_signbit(x) ? 1.0f16 : 0.0f16, y);
  }

  if (__abacus_isnan(y)) {
    return y;
  }

  // Sort out the double infinity case:
  if (__abacus_isinf(x) && __abacus_isinf(y)) {
    const abacus_half ansInf = (x > 0.0f16) ? 0.25f16 : 0.75f16;
    return __abacus_copysign(ansInf, y);
  }

  if (x == 0.0f16) {
    return __abacus_copysign(0.5f16, y);
  }

  const abacus_short inverse = __abacus_fabs(y) >= (1.2f16 * __abacus_fabs(x));

  const abacus_half ratio = inverse ? (x / y) : (y / x);

  const abacus_half x2 = ratio * ratio;

  const abacus_half poly = atan2pi_horner_polynomial(x2);

  // Adding this small constant to the polynomial, then multiplying by 'ratio',
  // loses too much precision in cases where the other operands are large.
  // e.g, abacus_half ans = (extra_precision + poly) * ratio;'
  // Using 2 separate multiplies added together instead preserves accuracy
  // here.
  const abacus_half extra_precision = 1.5985965728759765625e-4f16;
  abacus_half ans = 0;
  if (__abacus_isftz()) {
    // We need to scale this calculation by 32 to avoid FTZ on some hardware.
    const abacus_half scaled_ratio = ratio * 32.0f16;
    ans = (extra_precision * scaled_ratio) + (poly * scaled_ratio);
    ans *= 0.03125f16;
  } else {
    ans = (extra_precision * ratio) + (poly * ratio);
  }

  if (__abacus_isftz()) {
    const abacus_half abs_ratio = __abacus_fabs(ratio);

    // There is a special case failure where x2 == 0.7646484375. In this case,
    // we need to set ans to +/-0.228759765625 (depending on the sign of ratio).
    if (abacus::detail::cast::as<abacus_ushort>(x2) == 0x3A1E) {
      ans = __abacus_copysign(
          abacus::detail::cast::as<abacus_half>(abacus_ushort(0x3352)), ratio);
    } else if (abs_ratio <= 0.186279f16) {
      // x * (0.3183144629001617431640625 + x * (-3.6907196044921875e-4 + x *
      // (-0.10223388671875)))
      //
      // The first number is single precision, which can be split into two FP16
      // numbers:
      //  0.318359375f16 and -4.491209e-5f16
      //
      // To avoid flushing to zero, we multiply the ratio by 8, then later
      // divide by 8. This requires us to pre-multiply the polynomial constants
      // by 8.

      // 0.318359375f x 8
      const abacus_half remaining_term = 0.318359375f16 * 8.0f16;

      // Compute horner polynomial.
      ans = abacus::internal::horner_polynomial(abs_ratio, _atan2piH_ftz);
      ans = remaining_term + ans;

      // Perform final multiplcation, then undo the scaling.
      ans = (abs_ratio * ans) * 0.125f16;

      // As atan(-ratio) = -atan(ratio), we need to copy the sign of ratio.
      ans = __abacus_copysign(ans, ratio);
    }
  }

  abacus_half pi_multiplication_factor = 0.0f16;

  if (inverse) {
    pi_multiplication_factor = __abacus_copysign(0.5f16, ans);
    ans = -ans;
  }

  if (x < 0.0f16) {
    // Check the sign of y. However, if y is -0 the answer is different compared
    // to +0, so we need to check the bits directly:
    pi_multiplication_factor =
        pi_multiplication_factor +
        ((abacus::detail::cast::as<abacus_ushort>(y) < sign_mask) ? 1.0f16
                                                                  : -1.0f16);
  }

  // Two weird but consistent bit pattern fixes. They rely on the answer as
  // opposed to the inputs. It seems to cause an error because the double
  // reference answer is just below a power of 2, such that when rounded to a
  // half it becomes a power of 2. This has the effect of doubling the perceived
  // ULP error. The power of 2 is always the same:
  //   0.125 = 2^-3 = 0x3001
  //   3.125e-2 = 2^-5 = 0x2801
  abacus_ushort abs_ans_ushort =
      abacus::detail::cast::as<abacus_ushort>(__abacus_fabs(ans));
  if (abs_ans_ushort == 0x2801 || abs_ans_ushort == 0x3001) {
    ans = abacus::detail::cast::as<abacus_half>(abacus_ushort(
        abacus::detail::cast::as<abacus_ushort>(ans) - abacus_ushort(1)));
  }

  return ans + pi_multiplication_factor;
}
#endif  // __CA_BUILTINS_HALF_SUPPORT
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_atan2pi(abacus_half y, abacus_half x) {
  return atan2pi_half(y, x);
}
abacus_half2 ABACUS_API __abacus_atan2pi(abacus_half2 y, abacus_half2 x) {
  return atan2pi_half(y, x);
}
abacus_half3 ABACUS_API __abacus_atan2pi(abacus_half3 y, abacus_half3 x) {
  return atan2pi_half(y, x);
}
abacus_half4 ABACUS_API __abacus_atan2pi(abacus_half4 y, abacus_half4 x) {
  return atan2pi_half(y, x);
}
abacus_half8 ABACUS_API __abacus_atan2pi(abacus_half8 y, abacus_half8 x) {
  return atan2pi_half(y, x);
}
abacus_half16 ABACUS_API __abacus_atan2pi(abacus_half16 y, abacus_half16 x) {
  return atan2pi_half(y, x);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_atan2pi(abacus_float y, abacus_float x) {
  return atan2pi<>(y, x);
}
abacus_float2 ABACUS_API __abacus_atan2pi(abacus_float2 y, abacus_float2 x) {
  return atan2pi<>(y, x);
}
abacus_float3 ABACUS_API __abacus_atan2pi(abacus_float3 y, abacus_float3 x) {
  return atan2pi<>(y, x);
}
abacus_float4 ABACUS_API __abacus_atan2pi(abacus_float4 y, abacus_float4 x) {
  return atan2pi<>(y, x);
}
abacus_float8 ABACUS_API __abacus_atan2pi(abacus_float8 y, abacus_float8 x) {
  return atan2pi<>(y, x);
}
abacus_float16 ABACUS_API __abacus_atan2pi(abacus_float16 y, abacus_float16 x) {
  return atan2pi<>(y, x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_atan2pi(abacus_double y, abacus_double x) {
  return atan2pi<>(y, x);
}
abacus_double2 ABACUS_API __abacus_atan2pi(abacus_double2 y, abacus_double2 x) {
  return atan2pi<>(y, x);
}
abacus_double3 ABACUS_API __abacus_atan2pi(abacus_double3 y, abacus_double3 x) {
  return atan2pi<>(y, x);
}
abacus_double4 ABACUS_API __abacus_atan2pi(abacus_double4 y, abacus_double4 x) {
  return atan2pi<>(y, x);
}
abacus_double8 ABACUS_API __abacus_atan2pi(abacus_double8 y, abacus_double8 x) {
  return atan2pi<>(y, x);
}
abacus_double16 ABACUS_API __abacus_atan2pi(abacus_double16 y,
                                            abacus_double16 x) {
  return atan2pi<>(y, x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
