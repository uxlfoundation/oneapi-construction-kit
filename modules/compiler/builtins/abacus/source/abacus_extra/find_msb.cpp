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
#include <abacus/abacus_extra.h>
#include <abacus/abacus_integer.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

using namespace abacus::detail;

namespace {
template <typename T, bool IS_SIGNED = TypeTraits<T>::is_signed>
struct helper;

template <typename T>
struct helper<T, false> {
  static typename TypeTraits<T>::SignedType _(const T x) {
    return cast::as<typename TypeTraits<T>::SignedType>(x);
  }
};

template <typename T>
struct helper<T, true> {
  static T _(const T x) {
    const T c = x < 0;

    const T notX = ~x;
    return __abacus_select(x, notX, c);
  }
};

template <typename T>
typename TypeTraits<T>::SignedType find_msb(const T t) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  const SignedType width = sizeof(typename TypeTraits<T>::ElementType) * 8;

  const SignedType result = width - __abacus_clz(helper<T>::_(t)) - 1;

  const SignedType c = t == 0;

  return __abacus_select(result, (SignedType)-1, c);
}
}  // namespace

#define DEF(TYPE)                                                \
  abacus_##TYPE ABACUS_API __abacus_find_msb(abacus_##TYPE x) {  \
    return find_msb<>(x);                                        \
  }                                                              \
  abacus_##TYPE ABACUS_API __abacus_find_msb(abacus_u##TYPE x) { \
    return find_msb<>(x);                                        \
  }

DEF(char)
DEF(char2)
DEF(char3)
DEF(char4)
DEF(char8)
DEF(char16)

DEF(short)
DEF(short2)
DEF(short3)
DEF(short4)
DEF(short8)
DEF(short16)

DEF(int)
DEF(int2)
DEF(int3)
DEF(int4)
DEF(int8)
DEF(int16)

DEF(long)
DEF(long2)
DEF(long3)
DEF(long4)
DEF(long8)
DEF(long16)
