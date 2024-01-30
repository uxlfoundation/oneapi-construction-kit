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
#include <abacus/internal/is_denorm.h>
#include <abacus/internal/is_integer_quick.h>

namespace {
template <typename T, unsigned N = TypeTraits<T>::num_elements>
struct helper;

template <typename T, unsigned N>
struct helper {
  typedef typename TypeTraits<T>::SignedType SignedType;
  static T _(const T x) {
    // Truncate to int and find difference to float
    const SignedType asInt = abacus::detail::cast::convert<SignedType>(x);
    const T truncated = abacus::detail::cast::convert<T>(asInt);
    const T diff = x - truncated;

    // Add 1 to truncated value if diff and x are positive
    const SignedType is_positive = (diff > 0.0) & (x >= 0.0);
    const T incremented = truncated + 1.0;
    T result = __abacus_select(truncated, incremented, is_positive);
    result = __abacus_copysign(result, x);  // 0.0 -> -0.0

    // Return the original input for INF, NaN, and floats which already
    // represent integers
    const SignedType identity =
        ~__abacus_isnormal(x) | abacus::internal::is_integer_quick(x);
    result = __abacus_select(result, x, identity);

    // Denormal numbers will be close to zero. So go to -0.0 if negative, and
    // 1.0 if positive.
    const SignedType is_denorm = abacus::internal::is_denorm(x);
    T denorm_round = __abacus_copysign((T)0.5, x) + 0.5;
    denorm_round = __abacus_copysign(denorm_round, x);  // 0.0 -> -0.0
    return __abacus_select(result, denorm_round, is_denorm);
  }
};

template <typename T>
struct helper<T, 1u> {
  typedef typename TypeTraits<T>::SignedType SignedType;
  static T _(const T x) {
    if (abacus::internal::is_denorm(x)) {
      // Denormal numbers will be close to zero. So go to -0.0 if negative, and
      // 1.0 if positive.
      const T zero_or_one = __abacus_copysign((T)0.5, x) + (T)0.5;
      return __abacus_copysign(zero_or_one, x);  // Turns 0.0 -> -0.0
    } else if (!__abacus_isnormal(x) || abacus::internal::is_integer_quick(x)) {
      // Return the original input for INF, NaN, and floats which already
      // represent integers
      return x;
    }

    // Truncate to int and find difference to float
    const SignedType asInt = abacus::detail::cast::convert<SignedType>(x);
    const T truncated = abacus::detail::cast::convert<T>(asInt);
    const T diff = x - truncated;

    // Add 1 to truncated value if diff and x are positive
    if (diff > 0.0 && x >= 0.0) {
      return truncated + (T)1.0;
    }
    // Turns 0.0 to -0.0
    return __abacus_copysign(truncated, x);
  }
};

template <typename T>
T ceil(const T x) {
  return helper<T>::_(x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_ceil(abacus_half x) { return ceil<>(x); }
abacus_half2 ABACUS_API __abacus_ceil(abacus_half2 x) { return ceil<>(x); }
abacus_half3 ABACUS_API __abacus_ceil(abacus_half3 x) { return ceil<>(x); }
abacus_half4 ABACUS_API __abacus_ceil(abacus_half4 x) { return ceil<>(x); }
abacus_half8 ABACUS_API __abacus_ceil(abacus_half8 x) { return ceil<>(x); }
abacus_half16 ABACUS_API __abacus_ceil(abacus_half16 x) { return ceil<>(x); }
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_ceil(abacus_float x) { return ceil<>(x); }
abacus_float2 ABACUS_API __abacus_ceil(abacus_float2 x) { return ceil<>(x); }
abacus_float3 ABACUS_API __abacus_ceil(abacus_float3 x) { return ceil<>(x); }
abacus_float4 ABACUS_API __abacus_ceil(abacus_float4 x) { return ceil<>(x); }
abacus_float8 ABACUS_API __abacus_ceil(abacus_float8 x) { return ceil<>(x); }
abacus_float16 ABACUS_API __abacus_ceil(abacus_float16 x) { return ceil<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_ceil(abacus_double x) { return ceil<>(x); }
abacus_double2 ABACUS_API __abacus_ceil(abacus_double2 x) { return ceil<>(x); }
abacus_double3 ABACUS_API __abacus_ceil(abacus_double3 x) { return ceil<>(x); }
abacus_double4 ABACUS_API __abacus_ceil(abacus_double4 x) { return ceil<>(x); }
abacus_double8 ABACUS_API __abacus_ceil(abacus_double8 x) { return ceil<>(x); }
abacus_double16 ABACUS_API __abacus_ceil(abacus_double16 x) {
  return ceil<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
