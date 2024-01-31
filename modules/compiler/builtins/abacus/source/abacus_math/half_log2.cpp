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

static ABACUS_CONSTANT abacus_float __codeplay_half_log2_coeff[4] = {
    1.44227f, -.724239f, .511461f, -.328609f};

namespace {
template <typename T>
T _(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  SignedType xExp;
  T xMant = __abacus_frexp(x, &xExp);

  const SignedType xMantTooSmall =
      __abacus_isless(xMant, (T)7.07106769084930419921875e-1f);

  xMant *= __abacus_select((T)1.0f, (T)2.0f, xMantTooSmall);
  xExp -= __abacus_select((SignedType)0, (SignedType)1, xMantTooSmall);

  xMant -= (T)1.0f;

  T log2_xMant = xMant * abacus::internal::horner_polynomial(
                             xMant, __codeplay_half_log2_coeff);

  T result = abacus::detail::cast::convert<T>(xExp) + log2_xMant;
  result = __abacus_select(x, result, __abacus_isfinite(x));
  result = __abacus_select(result, -ABACUS_INFINITY, x == 0.0f);
  return __abacus_select(result, ABACUS_NAN, x < 0.0f);
}
}  // namespace

#define DEF(TYPE) \
  TYPE ABACUS_API __abacus_half_log2(TYPE x) { return _(x); }

DEF(abacus_float)
DEF(abacus_float2)
DEF(abacus_float3)
DEF(abacus_float4)
DEF(abacus_float8)
DEF(abacus_float16)
