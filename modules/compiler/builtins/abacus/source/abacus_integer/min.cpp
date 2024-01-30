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

#include <abacus/abacus_detail_integer.h>

#define DEF(TYPE)                                \
  TYPE ABACUS_API __abacus_min(TYPE x, TYPE y) { \
    return abacus::detail::integer::min(x, y);   \
  }
#define DEF2(TYPE, TYPE2)                         \
  TYPE ABACUS_API __abacus_min(TYPE x, TYPE2 y) { \
    return abacus::detail::integer::min(x, y);    \
  }

DEF(abacus_char);
DEF(abacus_char2);
DEF(abacus_char3);
DEF(abacus_char4);
DEF(abacus_char8);
DEF(abacus_char16);

DEF2(abacus_char2, abacus_char);
DEF2(abacus_char3, abacus_char);
DEF2(abacus_char4, abacus_char);
DEF2(abacus_char8, abacus_char);
DEF2(abacus_char16, abacus_char);

DEF(abacus_uchar);
DEF(abacus_uchar2);
DEF(abacus_uchar3);
DEF(abacus_uchar4);
DEF(abacus_uchar8);
DEF(abacus_uchar16);

DEF2(abacus_uchar2, abacus_uchar);
DEF2(abacus_uchar3, abacus_uchar);
DEF2(abacus_uchar4, abacus_uchar);
DEF2(abacus_uchar8, abacus_uchar);
DEF2(abacus_uchar16, abacus_uchar);

DEF(abacus_short);
DEF(abacus_short2);
DEF(abacus_short3);
DEF(abacus_short4);
DEF(abacus_short8);
DEF(abacus_short16);

DEF2(abacus_short2, abacus_short);
DEF2(abacus_short3, abacus_short);
DEF2(abacus_short4, abacus_short);
DEF2(abacus_short8, abacus_short);
DEF2(abacus_short16, abacus_short);

DEF(abacus_ushort);
DEF(abacus_ushort2);
DEF(abacus_ushort3);
DEF(abacus_ushort4);
DEF(abacus_ushort8);
DEF(abacus_ushort16);

DEF2(abacus_ushort2, abacus_ushort);
DEF2(abacus_ushort3, abacus_ushort);
DEF2(abacus_ushort4, abacus_ushort);
DEF2(abacus_ushort8, abacus_ushort);
DEF2(abacus_ushort16, abacus_ushort);

DEF(abacus_int);
DEF(abacus_int2);
DEF(abacus_int3);
DEF(abacus_int4);
DEF(abacus_int8);
DEF(abacus_int16);

DEF2(abacus_int2, abacus_int);
DEF2(abacus_int3, abacus_int);
DEF2(abacus_int4, abacus_int);
DEF2(abacus_int8, abacus_int);
DEF2(abacus_int16, abacus_int);

DEF(abacus_uint);
DEF(abacus_uint2);
DEF(abacus_uint3);
DEF(abacus_uint4);
DEF(abacus_uint8);
DEF(abacus_uint16);

DEF2(abacus_uint2, abacus_uint);
DEF2(abacus_uint3, abacus_uint);
DEF2(abacus_uint4, abacus_uint);
DEF2(abacus_uint8, abacus_uint);
DEF2(abacus_uint16, abacus_uint);

DEF(abacus_long);
DEF(abacus_long2);
DEF(abacus_long3);
DEF(abacus_long4);
DEF(abacus_long8);
DEF(abacus_long16);

DEF2(abacus_long2, abacus_long);
DEF2(abacus_long3, abacus_long);
DEF2(abacus_long4, abacus_long);
DEF2(abacus_long8, abacus_long);
DEF2(abacus_long16, abacus_long);

DEF(abacus_ulong);
DEF(abacus_ulong2);
DEF(abacus_ulong3);
DEF(abacus_ulong4);
DEF(abacus_ulong8);
DEF(abacus_ulong16);

DEF2(abacus_ulong2, abacus_ulong);
DEF2(abacus_ulong3, abacus_ulong);
DEF2(abacus_ulong4, abacus_ulong);
DEF2(abacus_ulong8, abacus_ulong);
DEF2(abacus_ulong16, abacus_ulong);
