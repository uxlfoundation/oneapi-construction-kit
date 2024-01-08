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
#include <abacus/internal/horner_polynomial.h>
#include <abacus/internal/sqrt.h>

namespace {
template <typename T>
T acosh(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  const T y = x - 1.0f;

  T ex = y + abacus::internal::sqrt(y * (y + 2.0f));

  // This can overflow so we check for large values
  const SignedType cond1 = y > 2.0e16f;
  ex = __abacus_select(ex, x, cond1);

  const T ln = __abacus_log1p(ex);

  const T ln2 = 0.693147180559945309417232121458176568075500134360255254120680;
  const SignedType cond2 = x > 2.0e16f;
  const T result = __abacus_select(ln, ln + ln2, cond2);

  const SignedType cond3 = x < 1.0f;
  return __abacus_select(result, (T)ABACUS_NAN, cond3);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
// See acosh sollya script for derivation
static ABACUS_CONSTANT abacus_half _acoshH[3] = {
    1.4140625f16, -0.11505126953125f16, 1.80816650390625e-2f16};

// See scalar implementation for more details about the algorithm.
template <typename T>
T acosh_half(const T x) {
  using SignedType = typename TypeTraits<T>::SignedType;

  const T xBigBound = 11.7f16;
  const T xOverflowBound = 32768.0f16;

  // A small optimization for vectorized versions. Rather than call log multiple
  // times and select the right answer, we instead do a smaller branch to pick
  // the input value for log:
  T log_input = __abacus_select(x + abacus::internal::sqrt(x * x - 1.0f16), x,
                                SignedType(x >= xBigBound));
  log_input = __abacus_select(log_input, 2.0f16 * x,
                              SignedType(x >= xBigBound && x < xOverflowBound));

  T ans = __abacus_log(log_input);

  // For values of x where 2*x would overflow, instead add log(2), as 'ans' will
  // be just log(x).
  ans =
      __abacus_select(ans, ABACUS_LN2_H + ans, SignedType(x >= xOverflowBound));

  const T small_return =
      abacus::internal::sqrt(x - 1.0f16) *
      abacus::internal::horner_polynomial(x - T(1.0f16), _acoshH);

  ans = __abacus_select(ans, small_return, SignedType(x < T(2.0f16)));

  return ans;
}

template <>
abacus_half acosh_half(const abacus_half x) {
  if (x < 2.0f16) {
    return abacus::internal::sqrt(x - 1.0f16) *
           abacus::internal::horner_polynomial(x - 1.0f16, _acoshH);
  }

  // As x > 11.7, acosh(x) = log(x + sqrt(x^2 - 1)) converges to
  // acosh(x) = log(2x), as the - 1 becomes insignificant. Note that acosh is
  // undefined for negative values of x.
  //
  // However, for inputs >= 32768, 2 * x will overflow, so we use
  // acosh(x) = log(2) + log(x) instead. Note that we can't use this for
  // x > 11.7, as this is not precise enough.
  if (x >= 32768.0f16) {
    return ABACUS_LN2_H + __abacus_log(x);
  } else if (x >= 11.7f16) {
    return __abacus_log(2.0f16 * x);
  }

  return __abacus_log(x + abacus::internal::sqrt(x * x - 1.0f16));
}

abacus_half ABACUS_API __abacus_acosh(abacus_half x) { return acosh_half<>(x); }
abacus_half2 ABACUS_API __abacus_acosh(abacus_half2 x) {
  return acosh_half<>(x);
}
abacus_half3 ABACUS_API __abacus_acosh(abacus_half3 x) {
  return acosh_half<>(x);
}
abacus_half4 ABACUS_API __abacus_acosh(abacus_half4 x) {
  return acosh_half<>(x);
}
abacus_half8 ABACUS_API __abacus_acosh(abacus_half8 x) {
  return acosh_half<>(x);
}
abacus_half16 ABACUS_API __abacus_acosh(abacus_half16 x) {
  return acosh_half<>(x);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_acosh(abacus_float x) { return acosh<>(x); }
abacus_float2 ABACUS_API __abacus_acosh(abacus_float2 x) { return acosh<>(x); }
abacus_float3 ABACUS_API __abacus_acosh(abacus_float3 x) { return acosh<>(x); }
abacus_float4 ABACUS_API __abacus_acosh(abacus_float4 x) { return acosh<>(x); }
abacus_float8 ABACUS_API __abacus_acosh(abacus_float8 x) { return acosh<>(x); }
abacus_float16 ABACUS_API __abacus_acosh(abacus_float16 x) {
  return acosh<>(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_acosh(abacus_double x) { return acosh<>(x); }
abacus_double2 ABACUS_API __abacus_acosh(abacus_double2 x) {
  return acosh<>(x);
}
abacus_double3 ABACUS_API __abacus_acosh(abacus_double3 x) {
  return acosh<>(x);
}
abacus_double4 ABACUS_API __abacus_acosh(abacus_double4 x) {
  return acosh<>(x);
}
abacus_double8 ABACUS_API __abacus_acosh(abacus_double8 x) {
  return acosh<>(x);
}
abacus_double16 ABACUS_API __abacus_acosh(abacus_double16 x) {
  return acosh<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
