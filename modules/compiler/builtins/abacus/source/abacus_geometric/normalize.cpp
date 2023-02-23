// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_detail_geometric.h>
#include <abacus/abacus_geometric.h>

#define SCALAR_DEF(TYPE)                                  \
  TYPE __abacus_normalize(TYPE x) {                       \
    if (x == static_cast<TYPE>(0) || __abacus_isnan(x)) { \
      return x;                                           \
    }                                                     \
    return __abacus_copysign(static_cast<TYPE>(1.0), x);  \
  }

#define DEF(TYPE)                                   \
  TYPE __abacus_normalize(TYPE x) {                 \
    return abacus::detail::geometric::normalize(x); \
  }

#ifdef __CA_BUILTINS_HALF_SUPPORT
SCALAR_DEF(abacus_half);
DEF(abacus_half2);
DEF(abacus_half3);
DEF(abacus_half4);
#endif  // __CA_BUILTINS_HALF_SUPPORT

SCALAR_DEF(abacus_float);
DEF(abacus_float2);
DEF(abacus_float3);
DEF(abacus_float4);

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
SCALAR_DEF(abacus_double);
DEF(abacus_double2);
DEF(abacus_double3);
DEF(abacus_double4);
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
