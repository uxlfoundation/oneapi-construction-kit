// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_common.h>
#include <abacus/abacus_detail_common.h>

#define DEF(TYPE)                                         \
  TYPE __abacus_smoothstep(TYPE e0, TYPE e1, TYPE x) {    \
    return abacus::detail::common::smoothstep(e0, e1, x); \
  }
#define DEF2(TYPE, TYPE2)                                 \
  TYPE __abacus_smoothstep(TYPE2 e0, TYPE2 e1, TYPE x) {  \
    return abacus::detail::common::smoothstep(e0, e1, x); \
  }

#ifdef __CA_BUILTINS_HALF_SUPPORT
DEF(abacus_half);
DEF(abacus_half2);
DEF(abacus_half3);
DEF(abacus_half4);
DEF(abacus_half8);
DEF(abacus_half16);

DEF2(abacus_half2, abacus_half);
DEF2(abacus_half3, abacus_half);
DEF2(abacus_half4, abacus_half);
DEF2(abacus_half8, abacus_half);
DEF2(abacus_half16, abacus_half);
#endif  // __CA_BUILTINS_HALF_SUPPORT

DEF(abacus_float);
DEF(abacus_float2);
DEF(abacus_float3);
DEF(abacus_float4);
DEF(abacus_float8);
DEF(abacus_float16);

DEF2(abacus_float2, abacus_float);
DEF2(abacus_float3, abacus_float);
DEF2(abacus_float4, abacus_float);
DEF2(abacus_float8, abacus_float);
DEF2(abacus_float16, abacus_float);

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF(abacus_double);
DEF(abacus_double2);
DEF(abacus_double3);
DEF(abacus_double4);
DEF(abacus_double8);
DEF(abacus_double16);

DEF2(abacus_double2, abacus_double);
DEF2(abacus_double3, abacus_double);
DEF2(abacus_double4, abacus_double);
DEF2(abacus_double8, abacus_double);
DEF2(abacus_double16, abacus_double);
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
