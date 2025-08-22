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

#ifndef __ABACUS_INTERNAL_ADD_EXACT_H__
#define __ABACUS_INTERNAL_ADD_EXACT_H__

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>

namespace abacus {
namespace internal {
// Assumes exponent of x >= exponent of y
template <typename T>
inline T add_exact_unsafe(const T x, const T y, T *out_remainder) {
  const T s = x + y;
  const T z = s - x;

  *out_remainder = y - z;
  return s;
}

// Order of x and y does not matter
template <typename T>
inline T add_exact(const T x, const T y, T *out_remainder) {
  const T s = x + y;
  const T a = s - y;
  const T b = s - a;
  const T da = x - a;
  const T db = y - b;
  const T t = da + db;

  *out_remainder = t;
  return s;
}

template <typename T> inline void add_exact_unsafe(T *x, T *y) {
  T r1_lo{};
  const T r1_hi = add_exact_unsafe<T>(*x, *y, &r1_lo);
  *x = r1_hi;
  *y = r1_lo;
}

template <typename T> inline void add_exact(T *x, T *y) {
  T r1_lo{};
  const T r1_hi = add_exact<T>(*x, *y, &r1_lo);
  *x = r1_hi;
  *y = r1_lo;
}
} // namespace internal
} // namespace abacus

#endif //__ABACUS_INTERNAL_ADD_EXACT_H__
