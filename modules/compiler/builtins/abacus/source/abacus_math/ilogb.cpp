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
#include <abacus/abacus_integer.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/is_denorm.h>
#include <abacus/internal/logb_unsafe.h>

namespace {

// A helper info struct to carry around some constant types which are dependent
// on the length of the floating point type.
template <typename T>
struct ilogb_info {
  // The returned int value will be a vector of 32 bit integers and a length
  // matching the length of the floating point type T.
  using IntType =
      typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type;

  // NAN and logb0 status flags wrapper.
  static constexpr IntType nan() { return IntType(ABACUS_FP_ILOGBNAN); }
  static constexpr IntType zero() { return IntType(ABACUS_FP_ILOGB0); }
};

template <typename T>
typename ilogb_info<T>::IntType ilogb_helper_scalar(const T x) {
  static_assert(TypeTraits<T>::num_elements == 1,
                "This function should only be called on scalar types");
  using SignedType = typename TypeTraits<T>::SignedType;
  using ReturnType = typename ilogb_info<T>::IntType;
  using Shape = FPShape<T>;

  const T absX = __abacus_fabs(x);
  if (!__abacus_isfinite(x)) {
    return ilogb_info<T>::nan();
  }

  if (absX == Shape::ZERO()) {
    return ilogb_info<T>::zero();
  }

  if (abacus::internal::is_denorm(x)) {
    // Interpret the bit pattern as an integer.
    const SignedType xui = abacus::detail::cast::as<SignedType>(absX);

    // Count the leading zeros in just the mantissa (removing the sign+exponent
    // zeros as this is a denormal number).
    const SignedType signPlusExponentNumBits =
        Shape::Sign() + Shape::Exponent();
    const SignedType denorm_degree =
        __abacus_clz(xui) - signPlusExponentNumBits;

    const SignedType result = -denorm_degree - Shape::Bias();

    // Convert from SignedType (16, 32, or 64 bit) to the return type of 32 bit
    // int for all scalar input types.
    return abacus::detail::cast::convert<ReturnType>(result);
  }

  return abacus::detail::cast::convert<ReturnType>(
      abacus::internal::logb_unsafe(x));
}

template <typename T>
typename ilogb_info<T>::IntType ilogb_helper(const T x) {
  static_assert(TypeTraits<T>::num_elements != 1,
                "This function should only be called on vector types");
  using SignedType = typename TypeTraits<T>::SignedType;
  using ReturnType = typename ilogb_info<T>::IntType;
  using Shape = FPShape<T>;

  const T absX = __abacus_fabs(x);

  // Reinterpret floating point bitpattern as int.
  const SignedType xui = abacus::detail::cast::as<SignedType>(absX);

  // Count the leading zeros in just the mantissa (removing the sign+exponent
  // zeros as this is a denormal number).
  const SignedType signPlusExponentNumBits = Shape::Sign() + Shape::Exponent();
  const SignedType denorm_degree = __abacus_clz(xui) - signPlusExponentNumBits;
  const SignedType denormResult = -denorm_degree - Shape::Bias();

  const SignedType normResult = abacus::internal::logb_unsafe(x);

  // Convert the return value to a vector of 32 bit ints from a vector of 16,
  // 32, or 64 bits.
  ReturnType result = abacus::detail::cast::convert<ReturnType>(__abacus_select(
      normResult, denormResult, abacus::internal::is_denorm(x)));

  const ReturnType compareAsI32 =
      abacus::detail::cast::convert<ReturnType>(absX == Shape::ZERO());

  result = __abacus_select(result, ilogb_info<T>::zero(), compareAsI32);

  const ReturnType isInfAsI32 =
      abacus::detail::cast::convert<ReturnType>(__abacus_isfinite(x));

  // Return the result, or NAN if the input is inf.
  return __abacus_select(result, ilogb_info<T>::nan(), ~isInfAsI32);
}

}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_int ABACUS_API __abacus_ilogb(abacus_half x) {
  return ilogb_helper_scalar<>(x);
}
abacus_int2 ABACUS_API __abacus_ilogb(abacus_half2 x) {
  return ilogb_helper<>(x);
}
abacus_int3 ABACUS_API __abacus_ilogb(abacus_half3 x) {
  return ilogb_helper<>(x);
}
abacus_int4 ABACUS_API __abacus_ilogb(abacus_half4 x) {
  return ilogb_helper<>(x);
}
abacus_int8 ABACUS_API __abacus_ilogb(abacus_half8 x) {
  return ilogb_helper<>(x);
}
abacus_int16 ABACUS_API __abacus_ilogb(abacus_half16 x) {
  return ilogb_helper<>(x);
}
#endif

abacus_int ABACUS_API __abacus_ilogb(abacus_float x) {
  return ilogb_helper_scalar<>(x);
}
abacus_int2 ABACUS_API __abacus_ilogb(abacus_float2 x) {
  return ilogb_helper<>(x);
}
abacus_int3 ABACUS_API __abacus_ilogb(abacus_float3 x) {
  return ilogb_helper<>(x);
}
abacus_int4 ABACUS_API __abacus_ilogb(abacus_float4 x) {
  return ilogb_helper<>(x);
}
abacus_int8 ABACUS_API __abacus_ilogb(abacus_float8 x) {
  return ilogb_helper<>(x);
}
abacus_int16 ABACUS_API __abacus_ilogb(abacus_float16 x) {
  return ilogb_helper<>(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_int ABACUS_API __abacus_ilogb(abacus_double x) {
  return ilogb_helper_scalar<>(x);
}
abacus_int2 ABACUS_API __abacus_ilogb(abacus_double2 x) {
  return ilogb_helper<>(x);
}
abacus_int3 ABACUS_API __abacus_ilogb(abacus_double3 x) {
  return ilogb_helper<>(x);
}
abacus_int4 ABACUS_API __abacus_ilogb(abacus_double4 x) {
  return ilogb_helper<>(x);
}
abacus_int8 ABACUS_API __abacus_ilogb(abacus_double8 x) {
  return ilogb_helper<>(x);
}
abacus_int16 ABACUS_API __abacus_ilogb(abacus_double16 x) {
  return ilogb_helper<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
