// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_detail_geometric.h>
#include <abacus/abacus_geometric.h>

#define DEF(TYPE)                                        \
  TYPE __abacus_fast_normalize(TYPE x) {                 \
    return abacus::detail::geometric::fast_normalize(x); \
  }

#ifdef __CA_BUILTINS_HALF_SUPPORT
DEF(abacus_half);
DEF(abacus_half2);
DEF(abacus_half3);
DEF(abacus_half4);
#endif  // __CA_BUILTINS_HALF_SUPPORT

DEF(abacus_float);
DEF(abacus_float2);
DEF(abacus_float3);
DEF(abacus_float4);

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF(abacus_double);
DEF(abacus_double2);
DEF(abacus_double3);
DEF(abacus_double4);
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
