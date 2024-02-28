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
#include <abacus/abacus_detail_integer.h>
#include <abacus/abacus_integer.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/convert_helper.h>

namespace {

template <typename T, typename S>
inline T ldexp(const T x, const S n) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;
  typedef FPShape<T> Shape;

  const SignedType n_c = convert_sat<SignedType>(n);

  // Compute the sign of n_c (-1, +1). Beware: relational operators behave
  // differently for scalar (return 1 if true) and vector (return -1 if true).
  const SignedType n_s = [&]() -> SignedType {
    if constexpr (TypeTraits<T>::num_elements == 1) {
      return -(n_c < 0) | 1;
    } else {
      return (n_c < 0) | 1;
    }
  }();

  // We split n into n_1 and n_2 with n_1 + n_1 + n_2 == n and |n_1 - n_2| <= 1,
  // if possible, so that we are assured we do not have n_1 > 0 and n_2 < 0,
  // which could result in overflow in an intermediate result, and so that we do
  // not end up with either n_1 or n_2 large enough that pow(2, n) overflows or
  // underflows except when we want an infinite or zero result anyway. For this,
  // we calculate n_1 = round(n_c / 3.0) = (n_c + n_s) / 3.
  // However, we also need to ensure that ldexp(ldexp(x, n_1), n_1) does not
  // reduce precision due to a subnormal intermediate result, so we have a
  // minimum value for n_1 even if that results in |n_1 - n_2| > 1; the cases
  // that we need to avoid in the general case cannot arise here. Then compute
  // ldexp(x, n) as ldexp(ldexp(ldexp(x, n_1), n_1), n_2).
  const SignedType n_1_min =
      (SignedType(1) - SignedType(((abacus::detail::cast::as<UnsignedType>(x) &
                                    Shape::ExponentMask()) >>
                                   Shape::Mantissa()))) /
      2;
  const SignedType n_1 = __abacus_max(
      SignedType(abacus::detail::integer::add_sat(n_c, n_s) / 3), n_1_min);
  const SignedType n_2 = n_c - (n_1 * 2);

  // Construct pow(2, n_1) and pow(2, n_2) by building the floating point
  // representation directly, but clamp to the lowest and highest representable
  // exponent so that we are sure not to shift into the sign bit, or beyond. It
  // is okay if this ignores overflow and underflow as the repeated
  // multiplication below will take care of that anyway.
  const UnsignedType biased_n_1 =
      abacus::detail::cast::convert<UnsignedType>(
          __abacus_clamp(SignedType(n_1 + Shape::Bias()), SignedType(1),
                         SignedType((1 << Shape::Exponent()) - 2)))
      << Shape::Mantissa();
  const UnsignedType biased_n_2 =
      abacus::detail::cast::convert<UnsignedType>(
          __abacus_clamp(SignedType(n_2 + Shape::Bias()), SignedType(1),
                         SignedType((1 << Shape::Exponent()) - 2)))
      << Shape::Mantissa();

  const T factor_1 = abacus::detail::cast::as<T>(biased_n_1);
  const T factor_2 = abacus::detail::cast::as<T>(biased_n_2);

  // Now use ldexp(y, n) ==> y * factor allows for a direct translation of
  // ldexp(ldexp(ldexp(x, n_1), n_1), n_2).
  return x * factor_1 * factor_1 * factor_2;
}

}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_ldexp(abacus_half x, abacus_int n) {
  return ldexp<>(x, n);
}

abacus_half2 ABACUS_API __abacus_ldexp(abacus_half2 x, abacus_int2 n) {
  return ldexp<>(x, n);
}

abacus_half3 ABACUS_API __abacus_ldexp(abacus_half3 x, abacus_int3 n) {
  return ldexp<>(x, n);
}

abacus_half4 ABACUS_API __abacus_ldexp(abacus_half4 x, abacus_int4 n) {
  return ldexp<>(x, n);
}

abacus_half8 ABACUS_API __abacus_ldexp(abacus_half8 x, abacus_int8 n) {
  return ldexp<>(x, n);
}

abacus_half16 ABACUS_API __abacus_ldexp(abacus_half16 x, abacus_int16 n) {
  return ldexp<>(x, n);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_ldexp(abacus_float x, abacus_int n) {
  return ldexp<>(x, n);
}

abacus_float2 ABACUS_API __abacus_ldexp(abacus_float2 x, abacus_int2 n) {
  return ldexp<>(x, n);
}

abacus_float3 ABACUS_API __abacus_ldexp(abacus_float3 x, abacus_int3 n) {
  return ldexp<>(x, n);
}

abacus_float4 ABACUS_API __abacus_ldexp(abacus_float4 x, abacus_int4 n) {
  return ldexp<>(x, n);
}

abacus_float8 ABACUS_API __abacus_ldexp(abacus_float8 x, abacus_int8 n) {
  return ldexp<>(x, n);
}

abacus_float16 ABACUS_API __abacus_ldexp(abacus_float16 x, abacus_int16 n) {
  return ldexp<>(x, n);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_ldexp(abacus_double x, abacus_int n) {
  return ldexp<>(x, n);
}

abacus_double2 ABACUS_API __abacus_ldexp(abacus_double2 x, abacus_int2 n) {
  return ldexp<>(x, n);
}

abacus_double3 ABACUS_API __abacus_ldexp(abacus_double3 x, abacus_int3 n) {
  return ldexp<>(x, n);
}

abacus_double4 ABACUS_API __abacus_ldexp(abacus_double4 x, abacus_int4 n) {
  return ldexp<>(x, n);
}

abacus_double8 ABACUS_API __abacus_ldexp(abacus_double8 x, abacus_int8 n) {
  return ldexp<>(x, n);
}

abacus_double16 ABACUS_API __abacus_ldexp(abacus_double16 x, abacus_int16 n) {
  return ldexp<>(x, n);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
