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

#ifndef __ABACUS_INTERNAL_LDEXP_UNSAFE_H__
#define __ABACUS_INTERNAL_LDEXP_UNSAFE_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_type_traits.h>

namespace abacus {
namespace internal {
template <typename T, typename N>
inline T ldexp_unsafe(const T &x, const N &n) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef FPShape<T> Shape;

  // This conversion is unsafe due to over/underflow when converting an
  // int to a short
  const SignedType nShort = abacus::detail::cast::convert<SignedType>(n);

  // Adding the bias in later operations could cause the exponent to overflow
  // it's assigned 5 bits, however we make no checks for that since this
  // function is unsafe.
  const SignedType bias = Shape::Bias();

  // Split `n` in two so that values of n which result in a denormal will
  // behave correctly when adding the bias. Otherwise `n + bias` could result
  // in a negative value.

  // Create an new float with value 2^(n/2) by setting exponent bits set to
  // 'n/2' plus the bias.
  const SignedType low = (nShort / 2 + bias) << Shape::Mantissa();
  const T factor1 = abacus::detail::cast::as<T>(low);

  // Create a new float with value 2^(n-(n/2)) by setting exponent bits set
  // to 'n-(n/2)' plus the bias.
  const SignedType high = (nShort - (nShort / 2) + bias) << Shape::Mantissa();
  const T factor2 = abacus::detail::cast::as<T>(high);

  // ld_exp(x, n) = x * 2^n
  //              = x * 2^(n/2) * 2^(n-(n/2))
  // No overflow or underflow bounds checking as in safe ldexp
  return x * factor1 * factor2;
}
} // namespace internal
} // namespace abacus

#endif //__ABACUS_INTERNAL_LDEXP_UNSAFE_H__
