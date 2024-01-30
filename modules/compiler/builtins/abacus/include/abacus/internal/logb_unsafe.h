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

#ifndef __ABACUS_INTERNAL_LOGB_UNSAFE_H__
#define __ABACUS_INTERNAL_LOGB_UNSAFE_H__

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_type_traits.h>

namespace abacus {
namespace internal {

// get_unbiased_exponent implements the operation used by all of the logb_unsafe
// functions. Each function does the same operation with different type
// dependent data. We use the typetraits for each type T to work out the values
// for that type.
template <typename T>
typename TypeTraits<T>::SignedType get_unbiased_exponent(T x) {
  using SignedType = typename TypeTraits<T>::SignedType;
  using Shape = FPShape<T>;

  // For vectors we apply the exponent mask to each element of the vector
  // bitcast to a vector of integer types (type depends on the floating point
  // type T). If x is a scalar then the bitcast and mask will apply directly to
  // that.
  const SignedType only_exponent =
      detail::cast::as<SignedType>(x) & Shape::ExponentMask();

  // Shift the exponent so we have an integer representing the biased exponent.
  const SignedType biased_exponent_as_int = only_exponent >> Shape::Mantissa();

  // Return the unbiased exponent.
  return biased_exponent_as_int - Shape::Bias();
}

// Overloads for single precision float types.
inline abacus_int logb_unsafe(abacus_float x) {
  return get_unbiased_exponent(x);
}
inline abacus_int2 logb_unsafe(abacus_float2 x) {
  return get_unbiased_exponent(x);
}
inline abacus_int3 logb_unsafe(abacus_float3 x) {
  return get_unbiased_exponent(x);
}
inline abacus_int4 logb_unsafe(abacus_float4 x) {
  return get_unbiased_exponent(x);
}
inline abacus_int8 logb_unsafe(abacus_float8 x) {
  return get_unbiased_exponent(x);
}
inline abacus_int16 logb_unsafe(abacus_float16 x) {
  return get_unbiased_exponent(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
// Overloads for double precision types.
inline abacus_long logb_unsafe(abacus_double x) {
  return get_unbiased_exponent(x);
}
inline abacus_long2 logb_unsafe(abacus_double2 x) {
  return get_unbiased_exponent(x);
}
inline abacus_long3 logb_unsafe(abacus_double3 x) {
  return get_unbiased_exponent(x);
}
inline abacus_long4 logb_unsafe(abacus_double4 x) {
  return get_unbiased_exponent(x);
}
inline abacus_long8 logb_unsafe(abacus_double8 x) {
  return get_unbiased_exponent(x);
}
inline abacus_long16 logb_unsafe(abacus_double16 x) {
  return get_unbiased_exponent(x);
}

#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

// Overloads for half precision types.
#ifdef __CA_BUILTINS_HALF_SUPPORT

inline abacus_short logb_unsafe(abacus_half x) {
  return get_unbiased_exponent(x);
}
inline abacus_short2 logb_unsafe(abacus_half2 x) {
  return get_unbiased_exponent(x);
}
inline abacus_short3 logb_unsafe(abacus_half3 x) {
  return get_unbiased_exponent(x);
}
inline abacus_short4 logb_unsafe(abacus_half4 x) {
  return get_unbiased_exponent(x);
}
inline abacus_short8 logb_unsafe(abacus_half8 x) {
  return get_unbiased_exponent(x);
}
inline abacus_short16 logb_unsafe(abacus_half16 x) {
  return get_unbiased_exponent(x);
}

#endif

}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_LOGB_UNSAFE_H__
