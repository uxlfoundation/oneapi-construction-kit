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

#include <abacus/internal/payne_hanek.h>
#include <abacus/internal/sincos_approx.h>

namespace {
template <typename T>
T sin(const T x) {
  using SignedType = typename TypeTraits<T>::SignedType;

  typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type octet = 0;
  // Range reduction from 0 -> pi/4:
  const T xReduced = abacus::internal::payne_hanek(x, &octet);

  // we calculate both cos_approx and sin_approx anyway, so use sincos approx:
  T cosApprox;
  const T sinApprox = abacus::internal::sincos_approx(xReduced, &cosApprox);

  const SignedType cond1 =
      abacus::detail::cast::convert<SignedType>(((octet + 1) & 0x2) == 0);
  const T result = __abacus_select(cosApprox, sinApprox, cond1);

  // sign changes depending on octet:
  const SignedType cond2 =
      abacus::detail::cast::convert<SignedType>((octet & 0x4) == 0);
  return __abacus_select(-result, result, cond2);
}

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
T sin_half(const T x) {
  using SignedType = typename TypeTraits<T>::SignedType;

  SignedType octet = 0;
  // Range reduction from 0 -> pi/4:
  const T xReduced = abacus::internal::payne_hanek_half(x, &octet);

  // Only care about the last 3 bits of octet
  octet = octet & 0x7;

  // we calculate both cos_approx and sin_approx anyway, so use sincos approx:
  T cosApprox;
  const T sinApprox = abacus::internal::sincos_approx(xReduced, &cosApprox);

  const SignedType cond1 = (((octet + 1) & 0x3) < 2);

  T result = __abacus_select(cosApprox, sinApprox, cond1);

  // sign changes depending on octet:
  T ans =
      __abacus_select(result, -result, SignedType((x < 0.0f16) ^ (octet >= 4)));

  // When denormals are unavailable, we need to handle the smallest FP16 value
  // explicitly, as the horner polynomial will get flushed to zero.
  ans = __abacus_select(ans, x,
                        SignedType(SignedType(__abacus_isftz()) &&
                                   __abacus_fabs(x) == 6.103515625e-05));

  return ans;
}
#endif  // __CA_BUILTINS_HALF_SUPPORT
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_sin(abacus_half x) { return sin_half<>(x); }
abacus_half2 ABACUS_API __abacus_sin(abacus_half2 x) { return sin_half<>(x); }
abacus_half3 ABACUS_API __abacus_sin(abacus_half3 x) { return sin_half<>(x); }
abacus_half4 ABACUS_API __abacus_sin(abacus_half4 x) { return sin_half<>(x); }
abacus_half8 ABACUS_API __abacus_sin(abacus_half8 x) { return sin_half<>(x); }
abacus_half16 ABACUS_API __abacus_sin(abacus_half16 x) { return sin_half<>(x); }
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_sin(abacus_float x) { return sin<>(x); }
abacus_float2 ABACUS_API __abacus_sin(abacus_float2 x) { return sin<>(x); }
abacus_float3 ABACUS_API __abacus_sin(abacus_float3 x) { return sin<>(x); }
abacus_float4 ABACUS_API __abacus_sin(abacus_float4 x) { return sin<>(x); }
abacus_float8 ABACUS_API __abacus_sin(abacus_float8 x) { return sin<>(x); }
abacus_float16 ABACUS_API __abacus_sin(abacus_float16 x) { return sin<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_sin(abacus_double x) { return sin<>(x); }
abacus_double2 ABACUS_API __abacus_sin(abacus_double2 x) { return sin<>(x); }
abacus_double3 ABACUS_API __abacus_sin(abacus_double3 x) { return sin<>(x); }
abacus_double4 ABACUS_API __abacus_sin(abacus_double4 x) { return sin<>(x); }
abacus_double8 ABACUS_API __abacus_sin(abacus_double8 x) { return sin<>(x); }
abacus_double16 ABACUS_API __abacus_sin(abacus_double16 x) { return sin<>(x); }
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
