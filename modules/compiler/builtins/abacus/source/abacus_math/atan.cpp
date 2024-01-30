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

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
#include <abacus/internal/atan_unsafe.h>
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
#include <abacus/internal/horner_polynomial.h>

// see maple worksheet for coefficient derivation
static ABACUS_CONSTANT abacus_float __codeplay_atan_coeff[8] = {
    +9.9999988079071044921875E-1f,
    -0.333319907463473626293856118291f,
    +0.199697238983619980545819298405f,
    -0.140194809132715612864576547329f,
    +0.991429283401126848185472742533e-1f,
    -0.594863931587656421400356212058e-1f,
    +0.242524030827416328323097536789e-1f,
    -0.469327600641822505599531812951e-2f};

abacus_float ABACUS_API __abacus_atan(abacus_float x) {
  const bool recip_x = 1.0f < __abacus_fabs(x);

  if (recip_x) {
    x = 1.0f / x;
  }

  float result =
      x * abacus::internal::horner_polynomial(x * x, __codeplay_atan_coeff, 8);

  if (recip_x) {
    result = __abacus_copysign(ABACUS_PI_2_F, x) - result;
  }

  return result;
}

namespace {
template <typename T>
T ABACUS_API atan(T x) {
  const typename TypeTraits<T>::SignedType recip_x = (T)1.0f < __abacus_fabs(x);

  x = __abacus_select(x, (T)1.0f / x, recip_x);

  const T result =
      x * abacus::internal::horner_polynomial(x * x, __codeplay_atan_coeff);

  return __abacus_select(result, __abacus_copysign(ABACUS_PI_2_F, x) - result,
                         recip_x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
namespace {
// See atan sollya file for derivation
static ABACUS_CONSTANT abacus_half __codeplay_atan_half[4] = {
    0.99951171875f16, -0.31884765625f16, 0.1356201171875f16,
    -3.08074951171875e-2f16};

template <typename T>
T ABACUS_API atan_half(T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  // Using the identity atan(1/x) = pi/2 - atan(x), we can calculate atan(x) for
  // large x by first calculating 1/x, atan of this, and finally subtracting
  // from pi/2.
  // By doing this we only need to in theory estimate atan over the range [0,1],
  // and derive the other values from this.
  // However, for values slightly above 1, we lose enough precision getting 1/x
  // and using this in a polynomial that we stray outside the acceptable ulp
  // error.
  // To this end we instead only invert when working on values above 1.2. This
  // range extension doesn't effect the number of terms needed in the base
  // polynomial, so all's well

  const T xAbs = __abacus_fabs(x);

  SignedType inverse = (xAbs >= 1.2f16);

  x = __abacus_select(x, 1.0f16 / x, inverse);

  T ans = x * abacus::internal::horner_polynomial(x * x, __codeplay_atan_half);

  ans = __abacus_select(ans, __abacus_copysign(ABACUS_PI_2_H, ans) - ans,
                        inverse);

  // When denormals are unavailable, we need to handle the smallest FP16 value
  // explicitly, as the horner polynomial will get flushed to zero.
  ans = __abacus_select(
      ans, x,
      SignedType(SignedType(__abacus_isftz()) && xAbs == 6.103515625e-05));

  return ans;
}
}  // namespace

abacus_half ABACUS_API __abacus_atan(abacus_half x) { return atan_half<>(x); }
abacus_half2 ABACUS_API __abacus_atan(abacus_half2 x) { return atan_half<>(x); }
abacus_half3 ABACUS_API __abacus_atan(abacus_half3 x) { return atan_half<>(x); }
abacus_half4 ABACUS_API __abacus_atan(abacus_half4 x) { return atan_half<>(x); }
abacus_half8 ABACUS_API __abacus_atan(abacus_half8 x) { return atan_half<>(x); }
abacus_half16 ABACUS_API __abacus_atan(abacus_half16 x) {
  return atan_half<>(x);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float2 ABACUS_API __abacus_atan(abacus_float2 x) { return atan<>(x); }
abacus_float3 ABACUS_API __abacus_atan(abacus_float3 x) { return atan<>(x); }
abacus_float4 ABACUS_API __abacus_atan(abacus_float4 x) { return atan<>(x); }
abacus_float8 ABACUS_API __abacus_atan(abacus_float8 x) { return atan<>(x); }
abacus_float16 ABACUS_API __abacus_atan(abacus_float16 x) { return atan<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
namespace {
template <typename T>
T atanD(const T x) {
  return abacus::internal::atan_unsafe(x);
}
}  // namespace

abacus_double ABACUS_API __abacus_atan(abacus_double x) { return atanD<>(x); }
abacus_double2 ABACUS_API __abacus_atan(abacus_double2 x) { return atanD<>(x); }
abacus_double3 ABACUS_API __abacus_atan(abacus_double3 x) { return atanD<>(x); }
abacus_double4 ABACUS_API __abacus_atan(abacus_double4 x) { return atanD<>(x); }
abacus_double8 ABACUS_API __abacus_atan(abacus_double8 x) { return atanD<>(x); }
abacus_double16 ABACUS_API __abacus_atan(abacus_double16 x) {
  return atanD<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
