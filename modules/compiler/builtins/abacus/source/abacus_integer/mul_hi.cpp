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

#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_integer.h>
#include <abacus/abacus_type_traits.h>

template <typename T>
static T mul_hi(const T x, const T y) {
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;
  enum { SHIFT = (sizeof(typename TypeTraits<T>::ElementType) * 4) };

  const T mask = (((typename TypeTraits<T>::ElementType)0x1) << SHIFT) - 1;

  const T xHi = x >> (T)SHIFT;
  const T xLoSigned = x & mask;
  const UnsignedType xLo = abacus::detail::cast::as<UnsignedType>(xLoSigned);
  const T yHi = y >> (T)SHIFT;
  const T yLoSigned = y & mask;
  const UnsignedType yLo = abacus::detail::cast::as<UnsignedType>(yLoSigned);

  const UnsignedType lo = xLo * yLo;
  const T m1 = abacus::detail::cast::as<T>(xLo) * yHi;
  const T m2 = xHi * abacus::detail::cast::as<T>(yLo);
  const T hi = xHi * yHi;

  const UnsignedType loShifted = lo >> (UnsignedType)SHIFT;
  return hi + (m1 >> (T)SHIFT) + (m2 >> (T)SHIFT) +
         (((m1 & mask) + (m2 & mask) +
           abacus::detail::cast::as<T>(loShifted)) >>
          (T)SHIFT);
}

#define DEF(TYPE) \
  TYPE ABACUS_API __abacus_mul_hi(TYPE x, TYPE y) { return mul_hi<TYPE>(x, y); }

DEF(abacus_char)
DEF(abacus_char2)
DEF(abacus_char3)
DEF(abacus_char4)
DEF(abacus_char8)
DEF(abacus_char16)

DEF(abacus_uchar)
DEF(abacus_uchar2)
DEF(abacus_uchar3)
DEF(abacus_uchar4)
DEF(abacus_uchar8)
DEF(abacus_uchar16)

DEF(abacus_short)
DEF(abacus_short2)
DEF(abacus_short3)
DEF(abacus_short4)
DEF(abacus_short8)
DEF(abacus_short16)

DEF(abacus_ushort)
DEF(abacus_ushort2)
DEF(abacus_ushort3)
DEF(abacus_ushort4)
DEF(abacus_ushort8)
DEF(abacus_ushort16)

DEF(abacus_int)
DEF(abacus_int2)
DEF(abacus_int3)
DEF(abacus_int4)
DEF(abacus_int8)
DEF(abacus_int16)

DEF(abacus_uint)
DEF(abacus_uint2)
DEF(abacus_uint3)
DEF(abacus_uint4)
DEF(abacus_uint8)
DEF(abacus_uint16)

DEF(abacus_long)
DEF(abacus_long2)
DEF(abacus_long3)
DEF(abacus_long4)
DEF(abacus_long8)
DEF(abacus_long16)

DEF(abacus_ulong)
DEF(abacus_ulong2)
DEF(abacus_ulong3)
DEF(abacus_ulong4)
DEF(abacus_ulong8)
DEF(abacus_ulong16)
