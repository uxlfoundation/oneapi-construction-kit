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
#include <abacus/internal/horner_polynomial.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

template <typename T>
struct helper<T, abacus_float> {
  static T _(const T x) {
    const abacus_float polynomial[9] = {
        +0.999999984530f,    -0.333330722167f,    +0.199926035420f,
        -0.142035440289f,    +0.106405958967f,    -0.750364848983e-1f,
        +0.426844903103e-1f, -0.160645730104e-1f, +0.284892648503e-2f};

    return x * abacus::internal::horner_polynomial(x * x, polynomial);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
// see maple worksheet for polynomial derivation
static ABACUS_CONSTANT abacus_double polynomialD[19] = {
    .9999999999999998340,     -.3333333333332104022,
    .1999999999848175234,     -.1428571421113247119,
    .1111110916883810321,     -0.9090878098741775691e-1,
    0.7691977305006862393e-1, -0.6664176181110217976e-1,
    0.5868541985185417220e-1, -0.5205165784143110253e-1,
    0.4573431397908107694e-1, -0.3865242376313311675e-1,
    0.3010877688673059300e-1, -0.2053431542331908609e-1,
    0.1159598074713210748e-1, -0.5097546985621723960e-2,
    0.1612562456785227657e-2, -0.3235206349294761306e-3,
    0.3072764408780525257e-4};

template <typename T>
struct helper<T, abacus_double> {
  static T _(const T x) {
    return x * abacus::internal::horner_polynomial(x * x, polynomialD);
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T ABACUS_API atan2(const T x, const T y) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  const T xAbs = __abacus_fabs(x);
  const T yAbs = __abacus_fabs(y);

  const SignedType cond1 = xAbs < yAbs;

  T numerator = __abacus_select(yAbs, xAbs, cond1);
  T denominator = __abacus_select(xAbs, yAbs, cond1);

  T calc = helper<T>::_(numerator / denominator);

  calc = __abacus_select((T)ABACUS_PI_2 - calc, calc, cond1);

  const SignedType cond2 = __abacus_isinf(x) & __abacus_isinf(y);
  calc = __abacus_select(calc, (T)ABACUS_PI * 0.25f, cond2);

  T result = __abacus_copysign((T)ABACUS_PI_2, x);

  const SignedType cond3 = y < 0;
  result =
      __abacus_select(result, __abacus_copysign((T)ABACUS_PI - calc, x), cond3);

  const SignedType cond4 = y > 0;
  result = __abacus_select(result, __abacus_copysign(calc, x), cond4);

  const SignedType cond5 = __abacus_signbit(y);
  const T part = __abacus_select((T)0.0f, (T)ABACUS_PI, cond5);

  const SignedType cond6 = (xAbs == 0) & (yAbs == 0);
  result = __abacus_select(result, __abacus_copysign(part, x), cond6);

  const SignedType cond7 = __abacus_isnan(x) | __abacus_isnan(y);
  return __abacus_select(result, FPShape<T>::NaN(), cond7);
}

#ifdef __CA_BUILTINS_HALF_SUPPORT
// atan2 polynomial over range [0.0000000001, 1.2], see atan2 sollya script
static ABACUS_CONSTANT abacus_half _atan2H[5] = {
    1.0f16, -0.330078125f16, 0.175048828125f16, -7.48291015625e-2f16,
    1.532745361328125e-2f16};

template <typename T>
T atan2_half(const T y, const T x) {
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef FPShape<abacus_half> Shape;

  const UnsignedType sign_mask = Shape::SignMask();

  // We need to use a more accurate value of pi than we can fit in a half for
  // some return values, so we represent pi in 2 halfs instead of 1:
  const T pi = 3.140625f16;
  const T pi_lo = 0.0009676535897932384626433832f16;

  const T xAbs = __abacus_fabs(x);
  const T yAbs = __abacus_fabs(y);

  const SignedType inverse = yAbs >= (1.2f16 * xAbs);
  const T ratio = __abacus_select((y / x), (x / y), inverse);

  const T x2 = ratio * ratio;

  T ans = ratio * abacus::internal::horner_polynomial(x2, _atan2H);

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

  ans = (ans + (pi_multiplication_factor * pi)) +
        (pi_multiplication_factor * pi_lo);

  ans = __abacus_select(ans, __abacus_copysign(pi * 0.5f16, y), x == 0.0f16);

  T infinity_ans =
      __abacus_select(T(pi * 0.75f16), T(pi * 0.25f16), (x > 0.0f16));
  infinity_ans = __abacus_copysign(infinity_ans, y);
  ans = __abacus_select(ans, infinity_ans,
                        __abacus_isinf(x) && __abacus_isinf(y));

  ans = __abacus_select(ans, y, __abacus_isnan(y));

  // Zero input edge cases tested by OpenCL CTS:
  // atan2(-0, 0) -> -0
  // atan2(0, 0) -> 0
  // atan2(0, -0) -> PI
  // atan2(-0, -0) -> -PI
  const SignedType x_sign_set = __abacus_signbit(x);
  const T part = __abacus_select(T(0.0f16), T(ABACUS_PI_H), x_sign_set);

  const SignedType zero_inputs = (xAbs == 0.0f16) & (yAbs == 0.0f16);
  ans = __abacus_select(ans, __abacus_copysign(part, y), zero_inputs);

  return ans;
}

template <>
abacus_half atan2_half(const abacus_half y, const abacus_half x) {
  typedef FPShape<abacus_half> Shape;

  const abacus_ushort sign_mask = Shape::SignMask();

  const abacus_half xAbs = __abacus_fabs(x);
  const abacus_half yAbs = __abacus_fabs(y);

  if ((xAbs == 0.0f16) && (yAbs == 0.0f16)) {
    // Zero input edge cases tested by OpenCL CTS:
    // atan2(-0,0) -> -0
    // atan2(0,0) -> 0
    // atan2(0,-0) -> PI
    // atan2(-0, -0) -> -PI
    const abacus_half part = __abacus_signbit(x) ? ABACUS_PI_H : 0.0f16;
    return __abacus_copysign(part, y);
  }

  if (__abacus_isnan(y)) {
    return y;
  }

  // We need to use a more accurate value of pi than we can fit in a half for
  // some return values, so we represent pi in 2 halfs instead of 1:
  const abacus_half pi = 3.140625f16;
  const abacus_half pi_lo = 0.0009676535897932384626433832f16;

  // Sort out the double infinity case:
  if (__abacus_isinf(x) && __abacus_isinf(y)) {
    const abacus_half ansInf = (x > 0.0f16) ? 0.25f16 * pi : 0.75f16 * pi;
    return __abacus_copysign(ansInf, y);
  }

  if (x == 0.0f16) {
    return __abacus_copysign(pi * 0.5f16, y);
  }

  // Pretty much the same algorithm as atan here:
  const abacus_short inverse = yAbs >= (1.2f16 * xAbs);

  const abacus_half ratio = inverse ? (x / y) : (y / x);

  const abacus_half x2 = ratio * ratio;

  abacus_half ans = ratio * abacus::internal::horner_polynomial(x2, _atan2H);

  abacus_half pi_multiplication_factor = 0.0f16;

  if (inverse) {
    pi_multiplication_factor = __abacus_copysign(0.5f16, ans);
    ans = -ans;
  }

  if (x < 0.0f16) {
    // Check the sign of y. However ,if y is -0 the answer is different compared
    // to +0, so we need to check the bits directly:
    pi_multiplication_factor =
        pi_multiplication_factor +
        ((abacus::detail::cast::as<abacus_ushort>(y) < sign_mask) ? 1.0f16
                                                                  : -1.0f16);
  }

  return (ans + (pi_multiplication_factor * pi)) +
         (pi_multiplication_factor * pi_lo);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_atan2(abacus_half x, abacus_half y) {
  return atan2_half(x, y);
}
abacus_half2 ABACUS_API __abacus_atan2(abacus_half2 x, abacus_half2 y) {
  return atan2_half(x, y);
}
abacus_half3 ABACUS_API __abacus_atan2(abacus_half3 x, abacus_half3 y) {
  return atan2_half(x, y);
}
abacus_half4 ABACUS_API __abacus_atan2(abacus_half4 x, abacus_half4 y) {
  return atan2_half(x, y);
}
abacus_half8 ABACUS_API __abacus_atan2(abacus_half8 x, abacus_half8 y) {
  return atan2_half(x, y);
}
abacus_half16 ABACUS_API __abacus_atan2(abacus_half16 x, abacus_half16 y) {
  return atan2_half(x, y);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_atan2(abacus_float x, abacus_float y) {
  return atan2<>(x, y);
}
abacus_float2 ABACUS_API __abacus_atan2(abacus_float2 x, abacus_float2 y) {
  return atan2<>(x, y);
}
abacus_float3 ABACUS_API __abacus_atan2(abacus_float3 x, abacus_float3 y) {
  return atan2<>(x, y);
}
abacus_float4 ABACUS_API __abacus_atan2(abacus_float4 x, abacus_float4 y) {
  return atan2<>(x, y);
}
abacus_float8 ABACUS_API __abacus_atan2(abacus_float8 x, abacus_float8 y) {
  return atan2<>(x, y);
}
abacus_float16 ABACUS_API __abacus_atan2(abacus_float16 x, abacus_float16 y) {
  return atan2<>(x, y);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_atan2(abacus_double x, abacus_double y) {
  return atan2<>(x, y);
}
abacus_double2 ABACUS_API __abacus_atan2(abacus_double2 x, abacus_double2 y) {
  return atan2<>(x, y);
}
abacus_double3 ABACUS_API __abacus_atan2(abacus_double3 x, abacus_double3 y) {
  return atan2<>(x, y);
}
abacus_double4 ABACUS_API __abacus_atan2(abacus_double4 x, abacus_double4 y) {
  return atan2<>(x, y);
}
abacus_double8 ABACUS_API __abacus_atan2(abacus_double8 x, abacus_double8 y) {
  return atan2<>(x, y);
}
abacus_double16 ABACUS_API __abacus_atan2(abacus_double16 x,
                                          abacus_double16 y) {
  return atan2<>(x, y);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
