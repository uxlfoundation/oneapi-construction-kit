// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_common.h>
#include <abacus/abacus_detail_common.h>

#define DEF(TYPE) \
  TYPE __abacus_sign(TYPE x) { return abacus::detail::common::sign(x); }

#ifdef __CA_BUILTINS_HALF_SUPPORT
DEF(abacus_half);
DEF(abacus_half2);
DEF(abacus_half3);
DEF(abacus_half4);
DEF(abacus_half8);
DEF(abacus_half16);
#endif  //  __CA_BUILTINS_HALF_SUPPORT

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
#endif  //  __CA_BUILTINS_DOUBLE_SUPPORT
