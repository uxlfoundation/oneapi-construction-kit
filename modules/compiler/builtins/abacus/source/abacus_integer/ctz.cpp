// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_detail_integer.h>

#define DEF(TYPE)                                 \
  TYPE ABACUS_API __abacus_ctz(TYPE x) {          \
    return (TYPE)abacus::detail::integer::ctz(x); \
  }

DEF(abacus_char);
DEF(abacus_char2);
DEF(abacus_char3);
DEF(abacus_char4);
DEF(abacus_char8);
DEF(abacus_char16);

DEF(abacus_uchar);
DEF(abacus_uchar2);
DEF(abacus_uchar3);
DEF(abacus_uchar4);
DEF(abacus_uchar8);
DEF(abacus_uchar16);

DEF(abacus_short);
DEF(abacus_short2);
DEF(abacus_short3);
DEF(abacus_short4);
DEF(abacus_short8);
DEF(abacus_short16);

DEF(abacus_ushort);
DEF(abacus_ushort2);
DEF(abacus_ushort3);
DEF(abacus_ushort4);
DEF(abacus_ushort8);
DEF(abacus_ushort16);

DEF(abacus_int);
DEF(abacus_int2);
DEF(abacus_int3);
DEF(abacus_int4);
DEF(abacus_int8);
DEF(abacus_int16);

DEF(abacus_uint);
DEF(abacus_uint2);
DEF(abacus_uint3);
DEF(abacus_uint4);
DEF(abacus_uint8);
DEF(abacus_uint16);

DEF(abacus_long);
DEF(abacus_long2);
DEF(abacus_long3);
DEF(abacus_long4);
DEF(abacus_long8);
DEF(abacus_long16);

DEF(abacus_ulong);
DEF(abacus_ulong2);
DEF(abacus_ulong3);
DEF(abacus_ulong4);
DEF(abacus_ulong8);
DEF(abacus_ulong16);
