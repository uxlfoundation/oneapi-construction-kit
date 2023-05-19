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

#ifndef __ABACUS_INTERNAL_IS_ODD_H__
#define __ABACUS_INTERNAL_IS_ODD_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/logb_unsafe.h>

namespace abacus {
namespace internal {

// Returns true if the integer component of the floating point number is odd.
template <typename T>
inline typename TypeTraits<T>::SignedType is_odd(const T& x) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef FPShape<T> Shape;

  // Floating point type information.
  const SignedType mantissaMask = Shape::MantissaMask();
  const SignedType mantissaHiddenBit = Shape::LeastSignificantExponentBit();
  const SignedType numBits = Shape::NumBits();

  // Mantissa with previously implicit 1.0 now included
  const SignedType mantissa =
      ((abacus::detail::cast::as<SignedType>(x) & mantissaMask) |
       mantissaHiddenBit);

  // Find the exponent
  const SignedType unbiasedExp = abacus::internal::logb_unsafe(x);

  // We're using the exponent to shift, we can only shift by [0, <bits in type>)
  const SignedType validExp = (unbiasedExp >= 0) & (unbiasedExp < numBits);

  // Shifting our mantissa by the exponent means that the hidden bit now holds
  // the least significant bit of the integer component of the float. If this
  // is set it means that the number is odd.
  const SignedType hiddenBitMasked =
      (mantissa << unbiasedExp) & mantissaHiddenBit;

  // Truncates any remaining fractional bits, since we're only interested in
  // the last bit of integer component.
  const SignedType isOdd = hiddenBitMasked == mantissaHiddenBit;

  return validExp & isOdd;
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_IS_ODD_H__
