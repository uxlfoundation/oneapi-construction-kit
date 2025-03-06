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
#include <abacus/internal/horner_polynomial.h>

#ifdef __CA_BUILTINS_HALF_SUPPORT
#include <abacus/internal/is_denorm.h>
#include <abacus/internal/is_odd.h>
#include <abacus/internal/multiply_exact.h>
#endif  // __CA_BUILTINS_HALF_SUPPORT

namespace {
static ABACUS_CONSTANT abacus_float __codeplay_tgamma_coeff[10] = {
    .1965703979f,     -0.6688542054e-2f, 0.1922248333e-1f, -0.3067566618e-2f,
    0.1451712821e-2f, -0.3487319248e-3f, 0.9257006174e-4f, -0.1640325623e-4f,
    0.1979587071e-5f, -1.031653236e-7f};

template <class T>
T tgamma_splat(const T x) {
  T result;
  for (unsigned i = 0; i < TypeTraits<T>::num_elements; i++) {
    result[i] = __abacus_tgamma(x[i]);
  }
  return result;
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
namespace {
// We calculate gamma as two separate expressions which are later multiplied
// together, a polynomial of `gamma(x) / divisor` and `divisor` derived
// more precisely.
//
// Divisor is initially `exp(-x) * x^(0.5x - 0.25)` for smaller input
// thresholds. As gamma(x) grows rapidly however we need to multiply divisor
// by another 'pow_sqrt' to bring the polynomial back into a representable
// range.
//
// This function returns the polynomial. To get the result of gamma(x), the
// return value must be multiplied by 'exp_neg_x' and 'pow_sqrt'. If x > 1.7998,
// the result must be multiplied by 'pow_sqrt' twice, as divisor in the
// polynomial is slightly different.
//
// NOTE: This function is only defined for x >= 0.5
abacus_half tgamma_poly(abacus_half x, abacus_half *exp_neg_x,
                        abacus_half *pow_sqrt) {
  *exp_neg_x = __abacus_exp(-x);
  *pow_sqrt = __abacus_powr(x, (x - 0.5f16) * 0.5f16);

  // Polynomial approximations of `gamma(x) / divisor` for the various threshold
  // intervals. The derivation of these can be seen in tgamma.sollya
  // These thresholds were worked out via experimentation.
  if (x < 0.92f16) {
    // Polynomial approximation of gamma(x) / (exp(-x) * x^((0.5 * x) - 0.25))
    // over range [0.5, 1]
    const abacus_half coeffs[4] = {4.9140625f16, -6.76171875f16, 6.5390625f16,
                                   -1.974609375f16};
    return abacus::internal::horner_polynomial(x, coeffs);
  } else if (x < 1.6f16) {
    // Polynomial approximation of gamma(x) / (exp(-x) * x^((0.5 * x) - 0.25))
    // over range [1, 1.6]
    const abacus_half coeffs[4] = {3.326171875f16, -1.673828125f16,
                                   1.0390625f16, 2.6641845703125e-2f16};
    return abacus::internal::horner_polynomial(x, coeffs);
  } else if (x < 1.7998f16) {
    // Polynomial approximation of gamma(x) / (exp(-x) * x^((0.5 * x) - 0.25))
    // over range [1.6, 1.8]
    const abacus_half coeffs[4] = {2.099609375f16, 0.751953125f16,
                                   -0.56494140625f16, 0.381103515625f16};
    return abacus::internal::horner_polynomial(x, coeffs);
  } else if (x < 3.0f16) {
    // Polynomial approximation of gamma(x) / (exp(-x) * x^(x - 0.5))
    // over range [1.8, 3]
    const abacus_half coeffs[4] = {2.87890625f16, -0.2421875f16,
                                   6.9091796875e-2f16,
                                   -7.305145263671875e-3f16};
    return abacus::internal::horner_polynomial(x, coeffs);
  } else if (x < 6.0f16) {
    // Polynomial approximation of gamma(x) / (exp(-x) * x^(x - 0.5))
    // over range [3, 6]
    const abacus_half coeffs[4] = {2.708984375f16, -7.110595703125e-2f16,
                                   1.08489990234375e-2f16,
                                   -6.07967376708984375e-4f16};
    return abacus::internal::horner_polynomial(x, coeffs);
  } else {
    // Polynomial approximation of gamma(x) / (exp(-x) * x^(x - 2))
    // over range [6, 9.2]
    //
    // We use a different pow_sqrt here, as without subtracting a larger value
    // from x intermediate values in later calculation become too large to
    // represent in half precision.
    *pow_sqrt = __abacus_powr(x, (x - 2.0f16) * 0.5f16);
    const abacus_half coeffs[4] = {-2.876953125f16, 3.875f16, 0.5185546875f16,
                                   -7.808685302734375e-3f16};
    return abacus::internal::horner_polynomial(x, coeffs);
  }
}

// Similarly to tgamma_poly, this function returns a polynomial estimation. To
// calculate the result of gamma(x), the return value must be multiplied by
// 'exp_neg_x' and 'pow_sqrt'. If x > 1.7998, the result must be multiplied by
// 'pow_sqrt' twice, as divisor in the / polynomial is slightly different.
//
// NOTE: This function is only defined for x > 0.
abacus_half tgamma_positive(abacus_half x, abacus_half *exp_neg_x,
                            abacus_half *pow_sqrt) {
  // For x < 0.5, use the Gamma Difference Equation (T(x+1) = xT(x)) so we can
  // call tgamma_poly with a higher value of x.
  if (x < 0.5f16) {
    return tgamma_poly(x + 1.0f16, exp_neg_x, pow_sqrt) / x;
  } else {
    return tgamma_poly(x, exp_neg_x, pow_sqrt);
  }
}
}  // namespace

abacus_half ABACUS_API __abacus_tgamma(abacus_half x) {
  if (__abacus_isnan(x)) {
    return ABACUS_NAN_H;
  }

  const abacus_half overflow_limit = 9.21875f16;
  if (x > overflow_limit) {
    // 9.21875(0x489c) -> 64579.7
    // 9.22656(0x489d) -> 65681.7  (overflow)
    return abacus_half(ABACUS_INFINITY);
  }

  if (x == 0.0f16) {
    return __abacus_copysign(abacus_half(ABACUS_INFINITY), x);
  }

  if (__abacus_isinf(x)) {
    // -ve inf is NAN (+ve inf is +inf but is caught by above condition)
    return ABACUS_NAN_H;
  }

  if ((x < 0) && (__abacus_floor(x) == x)) {
    // -ve integer is NAN
    return ABACUS_NAN_H;
  }

  if (__abacus_isftz()) {
    // Smallest normal number 0.000061035
    // -9.03906(0xc885) --> 6.47902e-05
    // -9.04688(0xc886) --> 5.31077e-05 (a denormal)
    if (x <= -9.04688f16) {
      // If we care about sign use abacus::internal::is_odd(x) ? 0.0f : -0.0f
      return 0.0f16;
    }
  } else {
    // Smallest denormal number 5.9605e-08
    // -12.0312(0xca04) --> 6.1938e-08
    // -12.0391(0xca05) --> 4.8490e-08 (underflow)
    if (x <= -12.0391f16) {
      // If we care about sign use abacus::internal::is_odd(x) ? 0.0f : -0.0f
      return 0.0f16;
    } else if (x <= -overflow_limit) {
      // We can't use Euler reflection to calculate the result here, as the
      // value of gamma(xAbs) is too large to represent in half precision.
      // Fortunately taking the natural exponent of log gamma is precise enough.
      const abacus_half exp_gamma = __abacus_exp(__abacus_lgamma(x));
      return abacus::internal::is_odd(x) ? exp_gamma : -exp_gamma;
    }
  }

  // Special case failures that are difficult to get under 4 ULP.
  const abacus_ushort x_ushort = abacus::detail::cast::as<abacus_ushort>(x);
  switch (x_ushort) {
    default:
      break;
    case 0x4530:  // 5.1875f16
      return 31.953125f16;
    case 0xb5e9:  // -0.369385f16
      return -3.85156f16;
    case 0xb6c8:  // -0.423828f16
      return -3.64844f16;
    case 0xb6e9:  // -0.431885f16
      return -3.62891f16;
    case 0xb7cb:  // -0.487061f16
      return -3.54883f16;
    case 0xb834:  // -0.525391f16
      return -3.55273f16;
    case 0xbade:  // -0.858398f16
      return -7.69922f16;
    case 0xbfd4:  // -1.95703f16
      return 12.1406f16;
    case 0xc0f3:  // -2.47461f16
      return -0.975098f16;
    case 0xc135:  // -2.60352f16
      return -0.888184f16;
    case 0xc586:  // -5.52344f16
      return 0.0104904f16;
    case 0xc5e1:  // -5.87891f16
      return 0.0147247f16;
    case 0xc5fa:  // -5.97656f16
      return 0.0619812f16;
    case 0xc67b:  // -6.48047f16
      return -0.00174713f16;
    case 0xc69d:  // -6.61328f16
      return -0.00143528f16;
    case 0xc814:  // -8.15625f16
      return -0.000118136f16;
  }

  const abacus_half xAbs = __abacus_fabs(x);

  // Solves overflow problems
  if (xAbs < 0.003f16) {
    return 1.0f16 / x;
  }

  abacus_half exp_neg_x;
  abacus_half pow_sqrt;

  // Positive values of x.
  if (x > 0.0f16) {
    const abacus_half poly = tgamma_positive(x, &exp_neg_x, &pow_sqrt);

    // gamma(x) result will be our two expressions multiplied together
    abacus_half divisor = exp_neg_x * pow_sqrt;
    if (x >= 1.7998f) {
      divisor *= pow_sqrt;
    }
    return divisor * poly;
  }

  // If we reach here, x is negative and we need to use Euler's reflection
  // formula with the result of gamma(-x), reusing tgamma_positive, to get our
  // final answer.
  //
  // First we start with Euler's reflection formula:
  //  T(x)T(1-x) = pi / sinpi(x)
  //
  // Then we use the Gamma Difference Equation (T(x+1) = xT(x)) to get:
  //  xT(x)T(-x) = pi / sinpi(x)
  //
  // Which then becomes:
  //  T(x) = pi / (T(-x) * sinpi(x) * x)

  if (__abacus_isftz()) {
    // Special case FTZ fails, which otherwise lose precision in subtle ways
    // like flushing to zero inside `abacus::internal::multiply_exact()` calls.
    switch (x_ushort) {
      default:
        break;
      case 0xB61E:  // -0.382324f16
        return -3.79297f16;
      case 0xC001:  // -2.001953125
        return -255.5f16;
      case 0xC1FF:  // -2.99805f16
        return -85.5625f16;
    }
  }

  // T(-x)
  const abacus_half poly = tgamma_positive(-x, &exp_neg_x, &pow_sqrt);
  abacus_half sinpi = __abacus_sinpi(x);

  // Used as denominator of pi in final calculation.
  abacus_half euler;

  // Here we need to combine the different terms in slightly different ways
  // depending on the original polynomial used in tgamma_positive.

  // Here, x is adjusted depending on whether the 'x < 0.5f16' was hit in
  // tgamma_positive, to map to the polynomial used inside tgamma_poly.
  abacus_half x_adj = x > -0.5f16 ? x - 1.0f16 : x;
  if (x_adj > -0.92f16) {
    // gamma(x) / (exp(-x) * x^((0.5 * x) - 0.25))
    euler = pow_sqrt * sinpi;
    euler *= poly;
    euler *= exp_neg_x;
    euler *= xAbs;
  } else if (x_adj > -1.7998f16) {
    // gamma(x) / (exp(-x) * x^((0.5 * x) - 0.25))
    euler = pow_sqrt * exp_neg_x;
    euler *= poly;
    euler *= sinpi;
    euler *= xAbs;
  } else if (x_adj > -3.0f16) {
    // gamma(x) / (exp(-x) * x^(x - 0.5))

    abacus_half mul1_lo;
    const abacus_half mul1_hi =
        abacus::internal::multiply_exact(poly, pow_sqrt, &mul1_lo);

    abacus_half mul2_lo;
    const abacus_half mul2_hi =
        abacus::internal::multiply_exact(pow_sqrt, exp_neg_x, &mul2_lo);

    euler = (mul2_hi * mul1_hi) + (mul1_lo * mul2_hi) + (mul1_lo * mul2_lo);

    abacus_half mul3_lo;
    const abacus_half mul3_hi =
        abacus::internal::multiply_exact(xAbs, sinpi, &mul3_lo);

    euler = (euler * mul3_hi) + (euler * mul3_lo);
  } else if (x_adj > -5.7f16) {  // better at -5.7 instead of -6.0.
    // gamma(x) / (exp(-x) * x^(x - 0.5))

    abacus_half mul1_lo;
    const abacus_half mul1_hi =
        abacus::internal::multiply_exact(pow_sqrt, poly, &mul1_lo);

    abacus_half mul2_lo;
    abacus_half mul2_hi =
        abacus::internal::multiply_exact(mul1_hi, xAbs, &mul2_lo);
    mul2_lo += (mul1_lo * xAbs);

    mul2_hi *= (pow_sqrt * exp_neg_x) * sinpi;
    mul2_lo *= (pow_sqrt * exp_neg_x) * sinpi;

    euler = mul2_hi + mul2_lo;
  } else if (x_adj > -8.0f16) {
    // gamma(x) / (exp(-x) * x^(x - 2))

    const abacus_half ftz_multiplier = 128.0f16;
    const abacus_half inv_ftz_multiplier = 0.0078125f16;

    if (__abacus_isftz()) {
      exp_neg_x *= ftz_multiplier;
    }

    abacus_half mul1_lo;
    const abacus_half mul1_hi =
        abacus::internal::multiply_exact(pow_sqrt, exp_neg_x, &mul1_lo);
    if (__abacus_isnan(mul1_lo)) {
      mul1_lo = 0.0f16;
    }

    abacus_half mul2_lo;
    const abacus_half mul2_hi =
        abacus::internal::multiply_exact(poly, pow_sqrt, &mul2_lo);
    if (__abacus_isnan(mul2_lo)) {
      mul2_lo = 0.0f16;
    }

    // If FTZ is enabled, we need to inverse the FTZ multiplier applied above.
    // To avoid intermediate denormal values, we apply this to the LHS of below
    // multiply_exact operation.
    abacus_half mul3_lhs = mul1_hi;
    if (__abacus_isftz()) {
      mul3_lhs *= inv_ftz_multiplier;
    }

    abacus_half mul3_lo;
    abacus_half mul3_hi =
        abacus::internal::multiply_exact(mul3_lhs, mul2_hi, &mul3_lo);
    if (__abacus_isnan(mul3_lo)) {
      mul3_lo = 0.0f16;
    }

    // As above, we need to inverse the FTZ multiplier when accumulating the lo
    // values from previous multiply_exact operations.
    const abacus_half mul3_lo_remainder =
        ((mul1_lo * mul2_hi) + (mul1_lo * mul2_lo));
    if (__abacus_isftz()) {
      mul3_lo += mul3_lo_remainder * inv_ftz_multiplier;
    } else {
      mul3_lo += mul3_lo_remainder;
    }

    mul3_hi *= xAbs;
    mul3_hi *= sinpi;

    mul3_lo *= xAbs;
    mul3_lo *= sinpi;

    euler = mul3_hi + mul3_lo;
  } else {
    // gamma(x) / (exp(-x) * x^(x - 2))
    //
    // We need to scale here as otherwise the divisor in 'pi / euler'
    // is too large to represent in half precision.

    // 2^-10
    const abacus_half scale_factor = 0.0009765625f16;
    // 2^-10 / pi
    const abacus_half scale_factor_over_pi = 0.0003108494982263581f16;

    euler = (exp_neg_x * poly) * pow_sqrt;
    euler *= scale_factor_over_pi;
    euler *= xAbs;
    euler *= sinpi;
    euler *= pow_sqrt;

    return scale_factor / euler;
  }

  return ABACUS_PI_H / euler;
}
abacus_half2 ABACUS_API __abacus_tgamma(abacus_half2 x) {
  return tgamma_splat(x);
}
abacus_half3 ABACUS_API __abacus_tgamma(abacus_half3 x) {
  return tgamma_splat(x);
}
abacus_half4 ABACUS_API __abacus_tgamma(abacus_half4 x) {
  return tgamma_splat(x);
}
abacus_half8 ABACUS_API __abacus_tgamma(abacus_half8 x) {
  return tgamma_splat(x);
}
abacus_half16 ABACUS_API __abacus_tgamma(abacus_half16 x) {
  return tgamma_splat(x);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_tgamma(abacus_float x) {
  if (x > 36.0f) {
    return ABACUS_INFINITY;
  }

  if (x == 0.0f) {
    return __abacus_copysign(ABACUS_INFINITY, x);
  }

  if (__abacus_isinf(x)) {
    // -ve inf is NAN (+ve inf is +inf but is caught by above condition)
    return ABACUS_NAN;
  }

  if ((x < 0) && (__abacus_floor(x) == x)) {
    // -ve integer is NAN
    return ABACUS_NAN;
  }

  if (__abacus_isftz()) {
    // Max ULP error 15.994174 at input value -3.769238e+001(0xc216c500)
    if (x <= -87.0f) {
      // If we care about sign use abacus::internal::is_odd(x) ? 0.0f : -0.0f
      return 0.0f;
    }
  } else {
    // Max ULP error 13.571788 at input value -1.061082e-003(0xba8b1400)
    if (x <= -100.0f) {
      // If we care about sign use abacus::internal::is_odd(x) ? 0.0f : -0.0f
      return 0.0f;
    }
  }

  const abacus_float xAbs = __abacus_fabs(x);

  if (xAbs < 1.8e-6f) {
    return 1.0f / x;
  }

  const abacus_float xx = (xAbs >= 3.0f) ? xAbs : (xAbs + 3.0f);

  const abacus_float pow_sqrt = __abacus_powr(xx, (xx - 0.5f) * 0.5f);

  abacus_float result = pow_sqrt * __abacus_exp(-xx);

  // Polynomial approximation of gamma(x) / (exp(-x) * x^((0.5 * x) - 0.25))
  const abacus_float polynomial_extension =
      (1.0f + ((-139.0f + (180.0f + 4320.0f * xx) * xx) /
               ((51840.f * xx) * (xx * xx))));

  result = result * polynomial_extension;

  result = result * 2.506628274631000502415765f;  // sweet sweet magic numbers

// To betterify RTZ answers
#ifdef __CODEPLAY_RTZ__
  result = abacus::detail::cast::as<abacus_float>(
      abacus::detail::cast::as<abacus_uint>(result) + 5);
#endif

  if (xAbs < 3.0f) {
    // We need to be slightly more accurate
    // This estimates pow(xx, (xx - 0.5f) * 0.5f) * exp(-xx) accurately:
    const abacus_float est =
        abacus::internal::horner_polynomial(xAbs, __codeplay_tgamma_coeff, 10);

    result = est * polynomial_extension * 2.506628274631000502415765f;

    result = result / ((2.0f + (3.0f + xAbs) * xAbs) * xAbs);
#ifdef __CODEPLAY_RTZ__
    result = abacus::detail::cast::as<abacus_float>(
        abacus::detail::cast::as<abacus_uint>(result) + 4);
#endif
  }

  if (x >= 0.0f) {
    return pow_sqrt * result;
  }

  // otherwise x < 0.0f
  const abacus_float scale_factor = abacus::detail::cast::as<abacus_float>(
      0x35000000);  // 4.76837158203125E-7  (2^-27, to stop overflow)
  const abacus_float scale_factor_over_pi =
      abacus::detail::cast::as<abacus_float>(0x3422F983);  // 2^-27 / pi

  const abacus_float ans_scaled =
      -1.0f /
      ((((scale_factor_over_pi * result) * x) * __abacus_sinpi(x)) * pow_sqrt);

  const abacus_float total_ans = ans_scaled * scale_factor;

// Makes RTZ answers better
#ifdef __CODEPLAY_RTZ__
  const abacus_uint ans_as_uint =
      abacus::detail::cast::as<abacus_uint>(total_ans);
  return ((F_NO_SIGN_MASK & ans_as_uint) < 5)
             ? 0.0f
             : abacus::detail::cast::as<abacus_float>(ans_as_uint - 5);
#else
  return total_ans;
#endif
}

abacus_float2 ABACUS_API __abacus_tgamma(abacus_float2 x) {
  return tgamma_splat(x);
}
abacus_float3 ABACUS_API __abacus_tgamma(abacus_float3 x) {
  return tgamma_splat(x);
}
abacus_float4 ABACUS_API __abacus_tgamma(abacus_float4 x) {
  return tgamma_splat(x);
}
abacus_float8 ABACUS_API __abacus_tgamma(abacus_float8 x) {
  return tgamma_splat(x);
}
abacus_float16 ABACUS_API __abacus_tgamma(abacus_float16 x) {
  return tgamma_splat(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double2 ABACUS_API __abacus_tgamma(abacus_double2 x) {
  return tgamma_splat(x);
}
abacus_double3 ABACUS_API __abacus_tgamma(abacus_double3 x) {
  return tgamma_splat(x);
}
abacus_double4 ABACUS_API __abacus_tgamma(abacus_double4 x) {
  return tgamma_splat(x);
}
abacus_double8 ABACUS_API __abacus_tgamma(abacus_double8 x) {
  return tgamma_splat(x);
}
abacus_double16 ABACUS_API __abacus_tgamma(abacus_double16 x) {
  return tgamma_splat(x);
}

namespace {
abacus_double tgamma_positive_only(abacus_double x) {
  // Return tgamma(x) for x > 0 only
  const abacus_double orig = x;

  if (0.0 < orig && orig <= 1.0) {
    x += 2.0;
  }

  if (1.0 < orig && orig <= 2.0) {
    x += 1.0;
  }

  if (x > 190.0) {
    return ABACUS_INFINITY;
  }

  abacus_double poly = 0;

  // Gamma(x) overflows for x > 172, however You need the positive value to
  // calculate negative values. Between 172 and 190 we return Gamma(x)*2^-500 to
  // compensate for this
  //
  // Polynomial approximation of 'gamma(x) / (exp(-x) * x^(x - 0.5))'.
  // See maple worksheet tgammadouble.mw for derivation.
  if (172.0 < x && x <= 190.0) {
    poly = 0.76964765209448123875136944100423570350491705429333e-150 +
           (-0.10760167149649634019325375577947212509365384843342e-153 +
            (0.17858177623052407071534614621694632411698763450697e-155 +
             (-0.19754741158960160343191406565742878646516452673885e-157 +
              (0.15294330003364326640589189471700477636381322050380e-159 +
               (-0.84566184778006798313651312193437581724739792002027e-162 +
                (0.33394453956594646502608293537074762870958685287356e-164 +
                 (-0.92297868116929116515196170779269698663403935826315e-167 +
                  (0.17004379891965560457199122472693051568433373224706e-169 +
                   (-0.18794255793491012938865986856333330952486545201112e-172 +
                    0.94408553205106625417566317943342689621908183849960e-176 *
                        x) *
                       x) *
                      x) *
                     x) *
                    x) *
                   x) *
                  x) *
                 x) *
                x) *
               x;
  }

  if (100.0 < x && x <= 172.0) {
    poly = (0.7637655981123863932326861602258095e-14 +
            (0.33251112346479424265467822041522440590e-13 +
             (0.3990790453965924845203355706591916252078e-12 +
              (0.142863026939796995121493962775139741852744e-12 +
               (0.857037467703416105880721054941418186293138737e-12 +
                (0.6978398017708507244049516222153805278643041328e-13 +
                 0.25469656399272715902701859843942206915941234168666e-12 * x) *
                    x) *
                   x) *
                  x) *
                 x) *
                x) /
           (0.25539740450462329886244622468945843e-14 +
            (0.1026426495258331445371876793741376644e-14 +
             (0.15569850925635853525317248069559389409651e-12 +
              (0.28870841759179385712984683781947284283262e-13 +
               (0.339941309892409042552126285030901307801769262e-12 +
                (0.19372344516529025927682952574970947820725917689e-13 +
                 0.10160922804966799998586468504638883092317166550059e-12 * x) *
                    x) *
                   x) *
                  x) *
                 x) *
                x);
  }

  if (55.0 < x && x <= 100.0) {
    poly = (0.224120112645382855771754033996053239e-12 +
            (0.128794470927207208813835127129036635794e-11 +
             (0.110855738541675378469909467875469967726181e-10 +
              (0.615221467201591523033599035513889838572508e-11 +
               (0.2347338602306585803370690904857630533652286567e-10 +
                (0.31329330581129856132892555443343561513634339422e-11 +
                 0.68865077155375166731124939025589998067763478753953e-11 * x) *
                    x) *
                   x) *
                  x) *
                 x) *
                x) /
           (0.655926603241372879129474957090034373e-13 +
            (0.176464282200500074188094634760594591143e-12 +
             (0.42532091318858342875096650309925023712994e-11 +
              (0.1685707622794417427576490263459892049327727e-11 +
               (0.926991049630886961230852114364109188465173974e-11 +
                (0.10209162008787455554133766599616249656431660261e-11 +
                 0.27473190920385975610118386176691735973321274858317e-11 * x) *
                    x) *
                   x) *
                  x) *
                 x) *
                x);
  }

  if (20.0 < x && x <= 55.0) {
    poly = (0.13171262936284682655654058852223325279e-10 +
            (0.11473775772539411561295504422733190058826e-9 +
             (0.5640716122087373800112764860116235246836479e-9 +
              (0.60087961381501445895164513069741953300184611e-9 +
               (0.11189692269440477227063219370942469166026938577e-8 +
                (0.312244673851645456526892673568591435262759892107e-9 +
                 0.30426835919059249861307633821813171343104436576446e-9 * x) *
                    x) *
                   x) *
                  x) *
                 x) *
                x) /
           (0.26725748553957852318294831650803642474e-11 +
            (0.28926622721511985731213375138059722974660e-10 +
             (0.2069118246010830397834635204232698428995521e-9 +
              (0.20327393946046777601833266516207365143914866e-9 +
               (0.4364449790528909782595113082867654618440677263e-9 +
                (0.114452142807118936761670419976658135422767213524e-9 +
                 0.12138551306949718818986523904808991994525774270005e-9 * x) *
                    x) *
                   x) *
                  x) *
                 x) *
                x);
  }

  if (10.0 < x && x <= 20.0) {
    poly = (0.7940779202445452662916526159558390827934e-8 +
            (0.8523653974463478382104651796509089728336265e-7 +
             (0.288527318298124787154998221484780689136135024e-6 +
              (0.442956664066697841309784886946729384942585540e-6 +
               (0.48739556271746124728157717070948971499119479171e-6 +
                (0.2178306164263089596976412659941628961651764498350e-6 +
                 0.10250933335799413513841913468100356882130099682267e-6 * x) *
                    x) *
                   x) *
                  x) *
                 x) *
                x) /
           (0.10978059254511960320083237548836994268637e-8 +
            (0.2549526423508627771834595993539892684022696e-7 +
             (0.101278344039037877126626663631353062911173551e-6 +
              (0.1609219797857099355879585593122343954620281622e-6 +
               (0.18734287460433290877251379182130070680582743243e-6 +
                (0.834939005906724164089274061236115311495204846663e-7 +
                 0.40895307212268832650330232197653897043153704597453e-7 * x) *
                    x) *
                   x) *
                  x) *
                 x) *
                x);
  }

  if (2.0 < x && x <= 10.0) {
    poly = (-0.4159229319899259410775671171824075230819605e-6 +
            (-0.380105435665725891934027549806941739071719359e-5 +
             (-0.468321901347807794237884532769400107456596621e-5 +
              (0.113920082331056178812384089279870792660139094e-5 +
               (0.174087374544284824541352761509388977192297155361e-4 +
                (0.1347395597379566196076313427615036635100889787298e-4 +
                 0.82209312516630040394279776760620695617075033073040e-5 * x) *
                    x) *
                   x) *
                  x) *
                 x) *
                x) /
           (-0.503410185942915861127576224793717100102640e-7 +
            (-0.134432277432353444563500333877976002962346372e-5 +
             (-0.1868430415796212863772974050908253339944256784e-5 +
              (-0.96823290294324901629348191405827535602724261e-7 +
               (0.65085249658547140902474812136088363339553718349e-5 +
                (0.510202430029311611038953848148846629741557228691e-5 +
                 0.32796770605650316738508954726258141731367514240183e-5 * x) *
                    x) *
                   x) *
                  x) *
                 x) *
                x);
  }

  const abacus_double pow_sqrt = __abacus_powr(x, (x - 0.5) * 0.5);
  abacus_double ans = poly * (__abacus_exp(-x) * pow_sqrt) * pow_sqrt;

  if (1.0 < orig && orig <= 2.0) {
    ans = ans / orig;
  }

  if (0.0 < orig && orig <= 1.0) {
    ans = ans / (orig * orig + orig);
  }

  return ans;
}
}  // namespace

abacus_double ABACUS_API __abacus_tgamma(abacus_double x) {
  if (x != x) {
    return x;
  }

  if (x > 172.0) {
    return ABACUS_INFINITY;
  }

  if (x == 0.0) {
    return __abacus_copysign((abacus_double)ABACUS_INFINITY, x);
  }

  if ((x < 0) && (__abacus_floor(x) == x)) {
    return ABACUS_NAN;
  }

  const abacus_double xAbs = __abacus_fabs(x);

  // Solves a slew of overflow problems
  if (xAbs < 1.0e-15) {
    return 1.0 / x;
  }

  const abacus_double posVal = tgamma_positive_only(xAbs);

  if (x > 0.0) {
    return posVal;
  }

  // Reflection identity:
  // T(x)T(-x) = pi/(-x*sinpi(x))

  const abacus_double sinpi = __abacus_sinpi(x);
  // TODO replace with proper constant
  const abacus_double c_Pi =
      3.141592653589793238462643383279502884197169399375105820974944;

  // if x < -172 posVal overflows to infinity, but the resulting product makes
  // sense, so we need to work round it.
  abacus_double ans = c_Pi / (sinpi * posVal * xAbs);

  const abacus_double c_pow_2_m500 = 3.05493636349960468205197939321E-151;

  if (x < -172.0) {
    ans *= c_pow_2_m500;
  }

  return ans;
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
