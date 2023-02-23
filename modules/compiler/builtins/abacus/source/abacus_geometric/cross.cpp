// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_detail_geometric.h>
#include <abacus/abacus_geometric.h>

#define DEF(TYPE)                                  \
  TYPE __abacus_cross(TYPE x, TYPE y) {            \
    return abacus::detail::geometric::cross(x, y); \
  }

#ifdef __CA_BUILTINS_HALF_SUPPORT
DEF(abacus_half3);
DEF(abacus_half4);
#endif  // __CA_BUILTINS_HALF_SUPPORT

DEF(abacus_float3);
DEF(abacus_float4);

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF(abacus_double3);
DEF(abacus_double4);
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
