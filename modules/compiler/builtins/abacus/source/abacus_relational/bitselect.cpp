// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_detail_relational.h>
#include <abacus/abacus_relational.h>

#define DEF(TYPE)                                              \
  TYPE ABACUS_API __abacus_bitselect(TYPE x, TYPE y, TYPE z) { \
    return abacus::detail::relational::bitselect(x, y, z);     \
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

#ifdef __CA_BUILTINS_HALF_SUPPORT
DEF(abacus_half);
DEF(abacus_half2);
DEF(abacus_half3);
DEF(abacus_half4);
DEF(abacus_half8);
DEF(abacus_half16);
#endif

DEF(abacus_float);
DEF(abacus_float2);
DEF(abacus_float3);
DEF(abacus_float4);
DEF(abacus_float8);
DEF(abacus_float16);

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF(abacus_double);
DEF(abacus_double2);
DEF(abacus_double3);
DEF(abacus_double4);
DEF(abacus_double8);
DEF(abacus_double16);
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
