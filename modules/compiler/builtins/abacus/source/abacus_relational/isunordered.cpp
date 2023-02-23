// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_detail_relational.h>
#include <abacus/abacus_relational.h>

namespace {
template <typename T>
struct helper {
  typedef typename TypeTraits<T>::SignedType type;
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <>
struct helper<abacus_double> {
  typedef abacus_int type;
};
#endif // __CA_BUILTINS_DOUBLE_SUPPORT

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <>
struct helper<abacus_half> {
  typedef abacus_int type;
};
#endif
}  // namespace

#define DEF(TYPE)                                             \
  helper<TYPE>::type __abacus_isunordered(TYPE x, TYPE y) {   \
    return abacus::detail::cast::convert<helper<TYPE>::type>( \
        abacus::detail::relational::isunordered(x, y));       \
  }

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
