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

namespace {
template <typename T>
T atanh(const T x) {
  return (T)0.5f * (__abacus_log1p(x) - __abacus_log1p(-x));
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
// See atanh sollya script for derivation
static ABACUS_CONSTANT abacus_half _atanhH[3] = {1.0f16, 0.327880859375f16,
                                                 0.26416015625f16};

template <typename T>
T atanh_half(const T x) {
  // Both of these work, one has a divide, the other calls log twice. Divide is
  // probably faster:
  // return T(0.5f16) * (__abacus_log(T(1.0) + x) - __abacus_log(T(1.0) - x));
  T ans = 0.5f16 * (__abacus_log((1.0f16 + x) / (1.0f16 - x)));

  ans = __abacus_select(ans,
                        x * abacus::internal::horner_polynomial(x * x, _atanhH),
                        __abacus_fabs(x) < 0.5f16);

  return ans;
}

template <>
abacus_half atanh_half(const abacus_half x) {
  if (__abacus_fabs(x) < 0.5f16) {
    return x * abacus::internal::horner_polynomial(x * x, _atanhH);
  }

  // Both of these work, one has a divide, the other calls log twice. Divide is
  // probably faster:
  // return 0.5f16 * (log_h(1.0f16 + x) - log_h(1.0f16 - x));
  return 0.5f16 * (__abacus_log((1.0f16 + x) / (1.0f16 - x)));
}

abacus_half ABACUS_API __abacus_atanh(abacus_half x) { return atanh_half<>(x); }
abacus_half2 ABACUS_API __abacus_atanh(abacus_half2 x) {
  return atanh_half<>(x);
}
abacus_half3 ABACUS_API __abacus_atanh(abacus_half3 x) {
  return atanh_half<>(x);
}
abacus_half4 ABACUS_API __abacus_atanh(abacus_half4 x) {
  return atanh_half<>(x);
}
abacus_half8 ABACUS_API __abacus_atanh(abacus_half8 x) {
  return atanh_half<>(x);
}
abacus_half16 ABACUS_API __abacus_atanh(abacus_half16 x) {
  return atanh_half<>(x);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_atanh(abacus_float x) { return atanh<>(x); }
abacus_float2 ABACUS_API __abacus_atanh(abacus_float2 x) { return atanh<>(x); }
abacus_float3 ABACUS_API __abacus_atanh(abacus_float3 x) { return atanh<>(x); }
abacus_float4 ABACUS_API __abacus_atanh(abacus_float4 x) { return atanh<>(x); }
abacus_float8 ABACUS_API __abacus_atanh(abacus_float8 x) { return atanh<>(x); }
abacus_float16 ABACUS_API __abacus_atanh(abacus_float16 x) {
  return atanh<>(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_atanh(abacus_double x) { return atanh<>(x); }
abacus_double2 ABACUS_API __abacus_atanh(abacus_double2 x) {
  return atanh<>(x);
}
abacus_double3 ABACUS_API __abacus_atanh(abacus_double3 x) {
  return atanh<>(x);
}
abacus_double4 ABACUS_API __abacus_atanh(abacus_double4 x) {
  return atanh<>(x);
}
abacus_double8 ABACUS_API __abacus_atanh(abacus_double8 x) {
  return atanh<>(x);
}
abacus_double16 ABACUS_API __abacus_atanh(abacus_double16 x) {
  return atanh<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
