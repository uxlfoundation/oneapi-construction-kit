// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_detail_integer.h>

#define DEF(TYPE)                                   \
  TYPE __abacus_mad24(TYPE x, TYPE y, TYPE z) {     \
    return abacus::detail::integer::mad24(x, y, z); \
  }

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
