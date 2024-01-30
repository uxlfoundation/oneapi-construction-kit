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

#ifndef __ABACUS_INTERNAL_MULTIPLY_EXACT_UNSAFE_H__
#define __ABACUS_INTERNAL_MULTIPLY_EXACT_UNSAFE_H__

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct Helper;

template <typename T>
struct Helper<T, abacus_uint> {
  static const abacus_uint m = 0xFFFFF000u;
};

template <typename T>
struct Helper<T, abacus_ulong> {
  static const abacus_ulong m = 0xFFFFFFFFF8000000u;
};
}  // namespace

namespace abacus {
namespace internal {
template <typename T>
inline T multiply_exact_unsafe(const T x, const T y, T *out_remainder) {
  // TODO
  //  If fma is available on hardware this is probably faster
  //  float multiply = x*y;
  //  *remainder = fma(x,y, - multiply);

  typedef typename TypeTraits<T>::UnsignedType UnsignedType;

  const typename TypeTraits<UnsignedType>::ElementType mask =
      Helper<UnsignedType>::m;

  const T x_hi = abacus::detail::cast::as<T>(
      abacus::detail::cast::as<UnsignedType>(x) & mask);
  const T x_lo = x - x_hi;

  // same with y
  const T y_hi = abacus::detail::cast::as<T>(
      abacus::detail::cast::as<UnsignedType>(y) & mask);
  const T y_lo = y - y_hi;

  const T r1 = x * y;
  const T t1 = -r1 + x_hi * y_hi;
  const T t2 = t1 + x_hi * y_lo;
  const T t3 = t2 + x_lo * y_hi;

  *out_remainder = t3 + x_lo * y_lo;
  return r1;
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_MULTIPLY_EXACT_UNSAFE_H__
