// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
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
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

// We implement log10 using log2 and the logarithm rules.
//
// Following the logarithm rules log10(x) expands to:
// log10(x) = log2(x)/log2(10)
//
// Which can then be represented as:
// log10(x) = log2(x)* (1/log2(10))
//
// This transformation is to eliminate the division.
//
// The result of the constant multiply term "1 / log2(10)" is represented by the
// value 0.3010299956...
//

// abacus_half is only available when __CA_BUILTINS_HALF_SUPPORT is set.
#ifdef __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct helper<T, abacus_half> {
  static T _(const T x) {
    const abacus_half one_over_log2_10 = 0.30102999566f16;
    return __abacus_log2(x) * one_over_log2_10;
  }
};

#endif

template <typename T>
struct helper<T, abacus_float> {
  static T _(const T x) {
    const abacus_float one_over_log2_10 = 0.30102999566f;
    return __abacus_log2(x) * one_over_log2_10;
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct helper<T, abacus_double> {
  static T _(const T x) {
    const abacus_double one_over_log2_10 =
        0.301029995663981195213738894724493026768189881462108541310427;
    return __abacus_log2(x) * one_over_log2_10;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T ABACUS_API log10(const T x) {
  return helper<T>::_(x);
}
}  // namespace

// abacus_half is only available when __CA_BUILTINS_HALF_SUPPORT is set.
#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_log10(abacus_half x) { return log10<>(x); }
abacus_half2 ABACUS_API __abacus_log10(abacus_half2 x) { return log10<>(x); }
abacus_half3 ABACUS_API __abacus_log10(abacus_half3 x) { return log10<>(x); }
abacus_half4 ABACUS_API __abacus_log10(abacus_half4 x) { return log10<>(x); }
abacus_half8 ABACUS_API __abacus_log10(abacus_half8 x) { return log10<>(x); }
abacus_half16 ABACUS_API __abacus_log10(abacus_half16 x) { return log10<>(x); }
#endif

abacus_float ABACUS_API __abacus_log10(abacus_float x) { return log10<>(x); }
abacus_float2 ABACUS_API __abacus_log10(abacus_float2 x) { return log10<>(x); }
abacus_float3 ABACUS_API __abacus_log10(abacus_float3 x) { return log10<>(x); }
abacus_float4 ABACUS_API __abacus_log10(abacus_float4 x) { return log10<>(x); }
abacus_float8 ABACUS_API __abacus_log10(abacus_float8 x) { return log10<>(x); }
abacus_float16 ABACUS_API __abacus_log10(abacus_float16 x) {
  return log10<>(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_log10(abacus_double x) { return log10<>(x); }
abacus_double2 ABACUS_API __abacus_log10(abacus_double2 x) {
  return log10<>(x);
}
abacus_double3 ABACUS_API __abacus_log10(abacus_double3 x) {
  return log10<>(x);
}
abacus_double4 ABACUS_API __abacus_log10(abacus_double4 x) {
  return log10<>(x);
}
abacus_double8 ABACUS_API __abacus_log10(abacus_double8 x) {
  return log10<>(x);
}
abacus_double16 ABACUS_API __abacus_log10(abacus_double16 x) {
  return log10<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
