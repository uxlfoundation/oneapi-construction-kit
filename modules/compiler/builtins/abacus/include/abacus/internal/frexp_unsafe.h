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

#ifndef __ABACUS_INTERNAL_frexp_UNSAFE_H__
#define __ABACUS_INTERNAL_frexp_UNSAFE_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/logb_unsafe.h>

namespace abacus {
namespace internal {

template <typename T, typename N>
inline T frexp_unsafe(const T &x, N *n) {
  using IntVecType =
      typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type;
  using SignedType = typename TypeTraits<T>::SignedType;
  using Shape = FPShape<T>;
  using UnsignedElemType = typename Shape::ScalarUnsignedType;

  const IntVecType integerOutputAsI32 =
      abacus::detail::cast::convert<IntVecType>(
          abacus::internal::logb_unsafe(x) + 1);

  const SignedType xAs = abacus::detail::cast::as<SignedType>(x);

  // A combined mask of the mantissa and sign bits.
  const UnsignedElemType signAndMantissaMask =
      Shape::MantissaMask() | Shape::SignMask();

  const SignedType maskedXAs =
      ((xAs & signAndMantissaMask) | Shape::ZeroPointFive());
  const T result = abacus::detail::cast::as<T>(maskedXAs);

  // Convert the i32 int to the spec defined integer type we are expected to
  // output for type T. For doubles it is i64, for half i16 and for float i32.
  *n = abacus::detail::cast::convert<N>(integerOutputAsI32);
  return result;
}

}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_frexp_UNSAFE_H__
