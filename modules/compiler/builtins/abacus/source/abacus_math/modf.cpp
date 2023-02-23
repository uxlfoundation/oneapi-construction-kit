// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T>
inline T modf_helper_scalar(T x, T* out_whole_number) {
  static_assert(TypeTraits<T>::num_elements == 1,
                "Function should only be used for scalar types");

  const T whole_number = __abacus_trunc(x);
  *out_whole_number = whole_number;
  return __abacus_isinf(x) ? __abacus_copysign(T(0.0), x) : x - whole_number;
}

template <typename T>
inline T modf_helper_vector(T x, T* out_whole_number) {
  static_assert(TypeTraits<T>::num_elements != 1,
                "Function should only be used for vector types");

  const T whole_number = __abacus_trunc(x);
  *out_whole_number = whole_number;
  return __abacus_select(x - whole_number, __abacus_copysign(T(0.0), x),
                         __abacus_isinf(x));
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_modf(abacus_half x, abacus_half* o) {
  return modf_helper_scalar(x, o);
}
abacus_half2 ABACUS_API __abacus_modf(abacus_half2 x, abacus_half2* o) {
  return modf_helper_vector(x, o);
}
abacus_half3 ABACUS_API __abacus_modf(abacus_half3 x, abacus_half3* o) {
  return modf_helper_vector(x, o);
}
abacus_half4 ABACUS_API __abacus_modf(abacus_half4 x, abacus_half4* o) {
  return modf_helper_vector(x, o);
}
abacus_half8 ABACUS_API __abacus_modf(abacus_half8 x, abacus_half8* o) {
  return modf_helper_vector(x, o);
}
abacus_half16 ABACUS_API __abacus_modf(abacus_half16 x, abacus_half16* o) {
  return modf_helper_vector(x, o);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_modf(abacus_float x, abacus_float* o) {
  return modf_helper_scalar(x, o);
}
abacus_float2 ABACUS_API __abacus_modf(abacus_float2 x, abacus_float2* o) {
  return modf_helper_vector(x, o);
}
abacus_float3 ABACUS_API __abacus_modf(abacus_float3 x, abacus_float3* o) {
  return modf_helper_vector(x, o);
}
abacus_float4 ABACUS_API __abacus_modf(abacus_float4 x, abacus_float4* o) {
  return modf_helper_vector(x, o);
}
abacus_float8 ABACUS_API __abacus_modf(abacus_float8 x, abacus_float8* o) {
  return modf_helper_vector(x, o);
}
abacus_float16 ABACUS_API __abacus_modf(abacus_float16 x, abacus_float16* o) {
  return modf_helper_vector(x, o);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_modf(abacus_double x, abacus_double* o) {
  return modf_helper_scalar(x, o);
}
abacus_double2 ABACUS_API __abacus_modf(abacus_double2 x, abacus_double2* o) {
  return modf_helper_vector(x, o);
}
abacus_double3 ABACUS_API __abacus_modf(abacus_double3 x, abacus_double3* o) {
  return modf_helper_vector(x, o);
}
abacus_double4 ABACUS_API __abacus_modf(abacus_double4 x, abacus_double4* o) {
  return modf_helper_vector(x, o);
}
abacus_double8 ABACUS_API __abacus_modf(abacus_double8 x, abacus_double8* o) {
  return modf_helper_vector(x, o);
}
abacus_double16 ABACUS_API __abacus_modf(abacus_double16 x,
                                         abacus_double16* o) {
  return modf_helper_vector(x, o);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
