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
#include <abacus/internal/horner_polynomial.h>
#include <abacus/internal/rsqrt_unsafe.h>

// r = sqrt(x^2 + y^2), lo = min(x, y), hi = max(x, y)
// r = sqrt(lo^2 + hi^2)
// r = sqrt(hi^2 / hi^2 * (lo^2 + hi^2))
// r = sqrt(hi^2 * (lo^2 / hi^2 + hi^2 / hi^2))
// r = hi * sqrt(lo^2 / hi^2 + hi^2 / hi^2)
// r = hi * sqrt(lo^2 / hi^2 + 1)
// r = hi * sqrt((lo / hi)^2 + 1)

namespace {
// For non-half call the original function:
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct hypot_helper {
  using SignedType = typename TypeTraits<T>::SignedType;
  using UnsignedType = typename TypeTraits<T>::UnsignedType;

  inline static T _(const T x, const T y) {
    const T xAbs = __abacus_fabs(x);
    const T yAbs = __abacus_fabs(y);

    const SignedType c0 = xAbs < yAbs;
    const T lo = __abacus_select(yAbs, xAbs, c0);
    const T hi = __abacus_select(xAbs, yAbs, c0);

    const T part = lo / hi;

    const T part2 = part * part + 1.0f;

    // See rsqrt.cpp for documentation on this bound and the scaling
    // done before calling rqsrt_unsafe().
    const SignedType xBig =
        (abacus::detail::cast::as<UnsignedType>(part2) >= 0x7e6eb50e);

    const T scaledPart2 =
        __abacus_select(part2, part2 * T(0.0625f), SignedType(xBig));

    T result = abacus::internal::rsqrt_unsafe(scaledPart2) * scaledPart2;

    result = __abacus_select(result, result * T(4.0f), SignedType(xBig));

    result = hi * result;

    const SignedType c1 = (xAbs == 0.0f) & (yAbs == 0.0f);
    result = __abacus_select(result, (T)0.0f, c1);

    const SignedType c2 = __abacus_isnan(x) | __abacus_isnan(y);
    result = __abacus_select(result, FPShape<T>::NaN(), c2);

    const SignedType c3 = __abacus_isinf(x) | __abacus_isinf(y);
    result = __abacus_select(result, (T)ABACUS_INFINITY, c3);

    return result;
  }
};

#ifdef __CA_BUILTINS_HALF_SUPPORT
// See hypot sollya script for derivation
static ABACUS_CONSTANT abacus_half __codeplay_hypot_coeff_half[5] = {
    1.0f16, -3.0517578125e-3f16, 0.53173828125f16, -0.10015869140625f16,
    -1.4434814453125e-2f16};

template <typename T>
struct hypot_helper<T, abacus_half> {
  using SignedType = typename TypeTraits<T>::SignedType;
  using UnsignedType = typename TypeTraits<T>::UnsignedType;
  using Shape = FPShape<T>;

  static T _(const T x, const T y) {
    const T xAbs = __abacus_fabs(x);
    const T yAbs = __abacus_fabs(y);

    T ans;
    if (__abacus_usefast()) {
      // Uses a divide operation rather than __abacus_sqrt(), but doesn't meet
      // the 2 ULP bounds required by the spec for all inputs
      // inputs, e.g hypot(0xa051 /* -0.00843048 */, 0xa7b4 /* -0.0300903 */)
      ans = fast(xAbs, yAbs);
    } else {
      // Uses slower __abacus_sqrt() which is currently implemented with
      // some 32-bit float operations in abacus::internal::sqrt
      ans = accurate(xAbs, yAbs);
    }

    // We need to do a check for infinities, NAN, and a (0,0) input.
    // The spec says that a value of infinity overwrites nan's:
    // Aka hypot(infinity, nan) = infinity so the infinity check is
    // performed last

    const SignedType zero_cond = (xAbs == 0.0f16) & (yAbs == 0.0f16);
    ans = __abacus_select(ans, T(0.0f16), zero_cond);

    const SignedType nan_cond = __abacus_isnan(x) | __abacus_isnan(y);
    ans = __abacus_select(ans, FPShape<T>::NaN(), nan_cond);

    const SignedType inf_cond = __abacus_isinf(x) | __abacus_isinf(y);
    ans = __abacus_select(ans, T(ABACUS_INFINITY), inf_cond);

    return ans;
  }

  // At the time of writing sqrt() uses some 32-bit float operations, so this
  // alternative divide method exists as a faster on hardware implementation
  // of hypot. It does not however meet the 2 ULP precision requirements of
  // OpenCL cl_khr_fp16 full profile for all inputs.
  static T fast(const T xAbs, const T yAbs) {
    // Get the max of x and y:
    const T max_xy = __abacus_select(yAbs, xAbs, SignedType(xAbs > yAbs));
    const T min_xy = __abacus_select(xAbs, yAbs, SignedType(xAbs > yAbs));

    // We now have sqrt(x^2 + y^2) = max_xy * sqrt(1.0 + (min_xy / max_xy)^2)
    // We also know that 0 <= min_xy / max_xy <= 1, as we picked (min_xy,
    // max_xy) such that min_xy <= max_xy
    //
    // So now we simply estimate sqrt(1 + x^2) with a polynomial, which for 16
    // bits is pretty short and avoids calling sqrt(x).
    //
    // We do use a divide in this algorithm, but it does save a lot of other
    // operations when calling sqrt(x) however the loss of precision from the
    // divide can magnify when estimating x^2.
    const T xReduced = min_xy / max_xy;

    // Estimate sqrt(1 + x^2) from x in [0,1]:
    // P = fpminimax(sqrt(1 + x^2), 4, [|11...|],[0;1],floating,relative);
    const T sqrt_guess = abacus::internal::horner_polynomial(
        xReduced, __codeplay_hypot_coeff_half);

    return max_xy * sqrt_guess;
  }

  // hypot() implementation using `sqrt()` instead of divide.
  // Essentially we just scale the inputs to get them into a workable range so
  // we can just do a sqrt(x*x + y*y)
  //
  // At the time of writing sqrt uses some 32-bit float operations, so the
  // divide method also exists as a faster on hardware, but less precise
  // alternative.
  static T accurate(const T xAbs, const T yAbs) {
    // We want to get our calculation into the range ~1.0
    // so we don't have to deal with underflow/overflow:
    // Get the larger of the exponents:
    const SignedType expX =
        (abacus::detail::cast::as<SignedType>(xAbs) >> Shape::Mantissa()) -
        Shape::Bias();
    const SignedType expY =
        (abacus::detail::cast::as<SignedType>(yAbs) >> Shape::Mantissa()) -
        Shape::Bias();

    SignedType expLarge = __abacus_select(expY, expX, SignedType(expX > expY));

    // The largest finite exponent possible in expLarge is 15, creating a
    // reduction factor `newHalf` of 2^-8 will clamp the result in
    // `{x,y}Reduced` to a maximum exponent of a 7. This prevents overflow
    // in the square operation where a magnitude greater than 2**8 for either
    // x or y would overflow.
    const SignedType expHigh(8);
    expLarge =
        __abacus_select(expLarge, expHigh, SignedType(expLarge > expHigh));

    // -14 is the smallest unbiased exponent for numbers in the normal range.
    // Clamping to this value prevents underflow in the square operation and
    // simplifies creating `newSameHalf` as we don't need to think about
    // denormals.
    const SignedType expLow(-14);
    expLarge = __abacus_select(expLarge, expLow, SignedType(expLarge < expLow));

    // Create a power of 2 with +- this exponent:
    const SignedType newSameHalf = (expLarge + Shape::Bias())
                                   << Shape::Mantissa();
    const SignedType newHalf = (-expLarge + Shape::Bias()) << Shape::Mantissa();

    const T similar_pow = abacus::detail::cast::as<T>(newSameHalf);

    const T inverse_pow = abacus::detail::cast::as<T>(newHalf);

    const T xReduced = xAbs * inverse_pow;
    const T yReduced = yAbs * inverse_pow;

    // NOTE: This call uses 32-bit float instruction as part of
    // abacus::internal::sqrt
    T ans = __abacus_sqrt(xReduced * xReduced + yReduced * yReduced);
    ans *= similar_pow;
    return ans;
  }
};
#endif  //__CA_BUILTINS_HALF_SUPPORT

template <typename T>
T hypot(const T x, const T y) {
  return hypot_helper<T>::_(x, y);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_hypot(abacus_half x, abacus_half y) {
  return hypot<>(x, y);
}
abacus_half2 ABACUS_API __abacus_hypot(abacus_half2 x, abacus_half2 y) {
  return hypot<>(x, y);
}
abacus_half3 ABACUS_API __abacus_hypot(abacus_half3 x, abacus_half3 y) {
  return hypot<>(x, y);
}
abacus_half4 ABACUS_API __abacus_hypot(abacus_half4 x, abacus_half4 y) {
  return hypot<>(x, y);
}
abacus_half8 ABACUS_API __abacus_hypot(abacus_half8 x, abacus_half8 y) {
  return hypot<>(x, y);
}
abacus_half16 ABACUS_API __abacus_hypot(abacus_half16 x, abacus_half16 y) {
  return hypot<>(x, y);
}
#endif

abacus_float ABACUS_API __abacus_hypot(abacus_float x, abacus_float y) {
  return hypot<>(x, y);
}
abacus_float2 ABACUS_API __abacus_hypot(abacus_float2 x, abacus_float2 y) {
  return hypot<>(x, y);
}
abacus_float3 ABACUS_API __abacus_hypot(abacus_float3 x, abacus_float3 y) {
  return hypot<>(x, y);
}
abacus_float4 ABACUS_API __abacus_hypot(abacus_float4 x, abacus_float4 y) {
  return hypot<>(x, y);
}
abacus_float8 ABACUS_API __abacus_hypot(abacus_float8 x, abacus_float8 y) {
  return hypot<>(x, y);
}
abacus_float16 ABACUS_API __abacus_hypot(abacus_float16 x, abacus_float16 y) {
  return hypot<>(x, y);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_hypot(abacus_double x, abacus_double y) {
  return hypot<>(x, y);
}
abacus_double2 ABACUS_API __abacus_hypot(abacus_double2 x, abacus_double2 y) {
  return hypot<>(x, y);
}
abacus_double3 ABACUS_API __abacus_hypot(abacus_double3 x, abacus_double3 y) {
  return hypot<>(x, y);
}
abacus_double4 ABACUS_API __abacus_hypot(abacus_double4 x, abacus_double4 y) {
  return hypot<>(x, y);
}
abacus_double8 ABACUS_API __abacus_hypot(abacus_double8 x, abacus_double8 y) {
  return hypot<>(x, y);
}
abacus_double16 ABACUS_API __abacus_hypot(abacus_double16 x,
                                          abacus_double16 y) {
  return hypot<>(x, y);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
