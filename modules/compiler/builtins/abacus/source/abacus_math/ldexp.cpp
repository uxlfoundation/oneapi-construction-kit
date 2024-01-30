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

#ifdef __CA_BUILTINS_HALF_SUPPORT
#include <abacus/internal/frexp_unsafe.h>
#endif
#include <abacus/internal/is_denorm.h>

namespace {

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T, typename S>
inline T ldexp_half(const T x, const S n) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef FPShape<T> Shape;

  // Split x into a mantissa with magnitude in range [0.5, 1) and an exponent.
  // In the case when n is close to the bias and the final result will be near
  // the minimum representable half, combining the exponent of x with n before
  // splitting into 3 parts preserves precision, as the second multiply by
  // mul_lo will stay above a denormal value.
  // Example case: ldexp(0.00291252f16 /* 0x19f7 */, -15).
  S exponent;
  const T x_fract = abacus::internal::frexp_unsafe(x, &exponent);
  const SignedType x_exp = abacus::detail::cast::convert<SignedType>(exponent);

  SignedType n_short = abacus::detail::cast::convert<SignedType>(n);
  const SignedType cond_frexp =
      (n_short == SignedType(-16)) | (n_short == SignedType(-15));
  n_short = __abacus_select(n_short, SignedType(n_short + x_exp), cond_frexp);

  // split n into 3 depending on how big it is, nShort may overflow here but
  // that scenario is taken into account by the bounds checking.
  const SignedType exp_bias = Shape::Bias();
  const SignedType cond = __abacus_abs(n_short) > (exp_bias - 1);
  const SignedType third = n_short / 3;
  SignedType n_hi = __abacus_select(n_short, third, cond);
  SignedType n_lo = __abacus_select(SignedType(0), third, cond);
  SignedType n_lo_lo = n_short - (n_hi + n_lo);

  // put these n's into exponents:
  n_hi += exp_bias;
  n_lo += exp_bias;
  n_lo_lo += exp_bias;

  // Shift n by exp bits, no mantissa is needed
  const SignedType mant_bits = Shape::Mantissa();
  const T mul_hi = abacus::detail::cast::as<T>(SignedType(n_hi << mant_bits));
  const T mul_lo = abacus::detail::cast::as<T>(SignedType(n_lo << mant_bits));
  const T mul_lo_lo =
      abacus::detail::cast::as<T>(SignedType(n_lo_lo << mant_bits));

  // Use fractional part of x if we've already split off it's exponent
  T result = __abacus_select(x * mul_hi, x_fract * mul_hi, cond_frexp);
  // Parens since floating point operations aren't associative, and any other
  // order would lose precision from x
  result = (result * mul_lo) * mul_lo_lo;

  // check if n is very big or very small:
  // The bound is the value of n necessary to go from the smallest number to the
  // biggest one:
  //   - 2 * 15: n to go from the smallest normal number to the biggest.
  //   - 9: n to go from the smallest denormal back into the normal range.
  //   (smallest denormal is 2 ^ -24, so 24 - 15 = 9).
  //   bound ==> (2 * 15) + 9 ==> 39
  const S normalize_n = 9;
  const S bias_as_int = abacus::detail::cast::convert<S>(exp_bias);
  const S bound = 2 * bias_as_int + normalize_n;

  // Underflow is defined as returning signed zero
  const SignedType cond2 =
      abacus::detail::cast::convert<SignedType>(n < -bound);
  const T copy_sign = __abacus_copysign(0.0f16, x);
  result = __abacus_select(result, copy_sign, cond2);

  // Overflow is defined as returning signed inf
  const SignedType inf = Shape::ExponentMask();
  const T half_infinity = abacus::detail::cast::as<T>(inf);
  const T copy_sign2 = __abacus_copysign(half_infinity, x);
  const SignedType cond3 = abacus::detail::cast::convert<SignedType>(n > bound);
  result = __abacus_select(result, copy_sign2, cond3);

  T return_value = x;
  const SignedType isnan = __abacus_isnan(x);

  // If flush to zero is supported and this is a denormal number then we should
  // flush to zero.
  const SignedType is_denorm = abacus::internal::is_denorm(x);
  if (__abacus_isftz()) {
    return_value = __abacus_select(return_value, (T)0.0f16, is_denorm);
    // Is denorm can be true for nan so ignore it in that case.
    return_value = __abacus_select(x, return_value, isnan);
  }

  const SignedType is_zero = (x == 0.0f16) & ~is_denorm;
  SignedType cond4 = __abacus_isinf(x) | isnan | is_zero;

  return __abacus_select(result, return_value, cond4);
}
#endif

template <typename T, typename SignedType = typename TypeTraits<T>::SignedType>
inline T ldexp_float(const T x, const SignedType n) {
  typedef FPShape<T> Shape;

  // split n into 3 depending on how big it is
  const abacus_int exp_bias = Shape::Bias();
  SignedType n_hi = __abacus_select(n, n / 3, __abacus_abs(n) > (exp_bias - 1));
  SignedType n_lo = __abacus_select(0, n_hi, __abacus_abs(n) > (exp_bias - 1));
  SignedType n_lo_lo = n - (n_hi + n_lo);

  // put these n's into exponents:
  n_hi += exp_bias;
  n_lo += exp_bias;
  n_lo_lo += exp_bias;

  const SignedType mant_bits = Shape::Mantissa();
  T mul_hi = abacus::detail::cast::as<T>(n_hi << mant_bits);
  T mul_lo = abacus::detail::cast::as<T>(n_lo << mant_bits);
  T mul_lo_lo = abacus::detail::cast::as<T>(n_lo_lo << mant_bits);

  T result = ((x * mul_hi) * mul_lo) * mul_lo_lo;

  // check if n is very big or very small:
  // The bound is the value of n necessary to go from the smallest number to the
  // biggest one:
  //   - 2 * 127: n to go from the smallest normal number to the biggest.
  //   - 22: n to go from the smallest denormal back into the normal range.
  //   (smallest denormal is 2 ^ -149, so 149 - 127 = 22).
  const abacus_int bound = 2 * exp_bias + 22;
  result = __abacus_select(result, __abacus_copysign(0.0f, x), n < -bound);

  result =
      __abacus_select(result, __abacus_copysign(ABACUS_INFINITY, x), n > bound);

  // ldexp should still work on denormals, even in FTZ mode, however comparing
  // a denormal number to 0 in FTZ mode will yield true, so we don't take into
  // account this comparison if the number is denormal. It also works fine
  // with denormal support since 0 is not denormal.
  const SignedType is_denorm = abacus::internal::is_denorm(x);
  return __abacus_select(
      result, x,
      __abacus_isinf(x) | __abacus_isnan(x) | ((x == 0.0f) & ~is_denorm));
}

template <typename T, typename S>
inline T ldexp_double(const T x, const S n) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef FPShape<T> Shape;

  const SignedType nLong = abacus::detail::cast::convert<SignedType>(n);
  const abacus_long exp_bias = Shape::Bias();
  // split n into 3 depending on how big it is
  SignedType n_hi =
      __abacus_select(nLong, nLong / 3, __abacus_abs(nLong) > (exp_bias - 1));
  SignedType n_lo =
      __abacus_select(0, n_hi, __abacus_abs(nLong) > (exp_bias - 1));
  SignedType n_lo_lo = nLong - (n_hi + n_lo);

  // Put these n's into exponents:
  n_hi += exp_bias;
  n_lo += exp_bias;
  n_lo_lo += exp_bias;

  const SignedType mant_bits = Shape::Mantissa();
  T mul_hi = abacus::detail::cast::as<T>(n_hi << mant_bits);
  T mul_lo = abacus::detail::cast::as<T>(n_lo << mant_bits);
  T mul_lo_lo = abacus::detail::cast::as<T>(n_lo_lo << mant_bits);

  T result = ((x * mul_hi) * mul_lo) * mul_lo_lo;

  // check if n is very big or very small:
  // The bound is the value of n necessary to go from the smallest number to the
  // biggest one:
  //   - 2 * 1023: n to go from the smallest normal number to the biggest.
  //   - 51: n to go from the smallest denormal back into the normal range.
  //   (smallest denormal is 2 ^ -1074, so 1074 - 1023 = 51).
  const abacus_long bound = 2 * exp_bias + 51;
  result = __abacus_select(result, __abacus_copysign(0.0, x), nLong < -bound);

  result = __abacus_select(result, __abacus_copysign(ABACUS_INFINITY, x),
                           nLong > bound);

  const SignedType is_denorm = abacus::internal::is_denorm(x);

  return __abacus_select(
      result, x,
      __abacus_isinf(x) | __abacus_isnan(x) | ((x == 0.0) & ~is_denorm));
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_ldexp(abacus_half x, abacus_int n) {
  return ldexp_half(x, n);
}

abacus_half2 ABACUS_API __abacus_ldexp(abacus_half2 x, abacus_int2 n) {
  return ldexp_half<>(x, n);
}

abacus_half3 ABACUS_API __abacus_ldexp(abacus_half3 x, abacus_int3 n) {
  return ldexp_half<>(x, n);
}

abacus_half4 ABACUS_API __abacus_ldexp(abacus_half4 x, abacus_int4 n) {
  return ldexp_half<>(x, n);
}

abacus_half8 ABACUS_API __abacus_ldexp(abacus_half8 x, abacus_int8 n) {
  return ldexp_half<>(x, n);
}

abacus_half16 ABACUS_API __abacus_ldexp(abacus_half16 x, abacus_int16 n) {
  return ldexp_half<>(x, n);
}
#endif

abacus_float ABACUS_API __abacus_ldexp(abacus_float x, abacus_int n) {
  return ldexp_float(x, n);
}

abacus_float2 ABACUS_API __abacus_ldexp(abacus_float2 x, abacus_int2 n) {
  return ldexp_float<>(x, n);
}

abacus_float3 ABACUS_API __abacus_ldexp(abacus_float3 x, abacus_int3 n) {
  return ldexp_float<>(x, n);
}

abacus_float4 ABACUS_API __abacus_ldexp(abacus_float4 x, abacus_int4 n) {
  return ldexp_float<>(x, n);
}

abacus_float8 ABACUS_API __abacus_ldexp(abacus_float8 x, abacus_int8 n) {
  return ldexp_float<>(x, n);
}

abacus_float16 ABACUS_API __abacus_ldexp(abacus_float16 x, abacus_int16 n) {
  return ldexp_float<>(x, n);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
// We don't use the templated version for scalar doubles because the return
// types of functions such as isnan are inconsistent for doubles, they return
// longs for vectors and ints for scalars.
abacus_double ABACUS_API __abacus_ldexp(abacus_double x, abacus_int n) {
  typedef FPShape<abacus_double> Shape;
  const bool is_not_denorm = 0 == abacus::internal::is_denorm(x);

  if (__abacus_isinf(x) || __abacus_isnan(x) || (x == 0.0 && is_not_denorm)) {
    return x;
  }

  // check if n is very big or very small:
  // The bound is the value of n necessary to go from the smallest number to the
  // biggest one:
  //   - 2 * 1023: n to go from the smallest normal number to the biggest.
  //   - 51: n to go from the smallest denormal back into the normal range.
  //   (smallest denormal is 2 ^ -1074, so 1074 - 1023 = 51).
  const abacus_int bias = (abacus_int)Shape::Bias();
  const abacus_int bound = 2 * bias + 51;
  if (n > bound) {
    return __abacus_copysign((abacus_double)ABACUS_INFINITY, x);
  }

  if (n < -bound) {
    return __abacus_copysign(0.0, x);
  }

  // split n into 3 depending on how big it is
  abacus_long n_hi =
      (__abacus_abs(n) > (bias - 1)) ? (abacus_long)n / 3 : (abacus_long)n;
  abacus_long n_lo = (__abacus_abs(n) > (bias - 1)) ? n_hi : (abacus_long)0;
  abacus_long n_lo_lo = n - (n_hi + n_lo);

  // put these n's into exponents:
  n_hi += bias;
  n_lo += bias;
  n_lo_lo += bias;

  const abacus_long mant_bits = Shape::Mantissa();
  const abacus_double mul_hi =
      abacus::detail::cast::as<abacus_double>(n_hi << mant_bits);
  const abacus_double mul_lo =
      abacus::detail::cast::as<abacus_double>(n_lo << mant_bits);
  const abacus_double mul_lo_lo =
      abacus::detail::cast::as<abacus_double>(n_lo_lo << mant_bits);

  return ((x * mul_hi) * mul_lo) * mul_lo_lo;
}

abacus_double2 ABACUS_API __abacus_ldexp(abacus_double2 x, abacus_int2 n) {
  return ldexp_double<>(x, n);
}

abacus_double3 ABACUS_API __abacus_ldexp(abacus_double3 x, abacus_int3 n) {
  return ldexp_double<>(x, n);
}

abacus_double4 ABACUS_API __abacus_ldexp(abacus_double4 x, abacus_int4 n) {
  return ldexp_double<>(x, n);
}

abacus_double8 ABACUS_API __abacus_ldexp(abacus_double8 x, abacus_int8 n) {
  return ldexp_double<>(x, n);
}

abacus_double16 ABACUS_API __abacus_ldexp(abacus_double16 x, abacus_int16 n) {
  return ldexp_double<>(x, n);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
