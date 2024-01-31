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
#include <abacus/abacus_extra.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

using namespace abacus::detail;

namespace {
template <typename T>
typename TypeTraits<T>::SignedType find_lsb(const T t) {
  typedef typename TypeTraits<T>::SignedType SignedType;

  // find_lsb is semantically identical to ctz, except in the edgecase that the
  // operand is zero.
  // ctz(0) = sizeof(0) * 8 i.e. the size of the type in bits.
  // lsb(0) = -1.
  // So here we use ctz to implement the non zero case and otherwise special
  // case 0.
  const SignedType c0 = t != 0;
  return __abacus_select((SignedType)(-1), integer::ctz(t), c0);
}
}  // namespace

#define DEF(TYPE)                                                \
  abacus_##TYPE ABACUS_API __abacus_find_lsb(abacus_##TYPE x) {  \
    return find_lsb<>(x);                                        \
  }                                                              \
  abacus_##TYPE ABACUS_API __abacus_find_lsb(abacus_u##TYPE x) { \
    return find_lsb<>(x);                                        \
  }

DEF(char);
DEF(char2);
DEF(char3);
DEF(char4);
DEF(char8);
DEF(char16);

DEF(short);
DEF(short2);
DEF(short3);
DEF(short4);
DEF(short8);
DEF(short16);

DEF(int);
DEF(int2);
DEF(int3);
DEF(int4);
DEF(int8);
DEF(int16);

DEF(long);
DEF(long2);
DEF(long3);
DEF(long4);
DEF(long8);
DEF(long16);
