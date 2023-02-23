// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>

namespace {
template <typename T>
inline T mad_helper(T x, T y, T z) {
  return x * y + z;
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_mad(abacus_half x, abacus_half y,
                                    abacus_half z) {
  return mad_helper(x, y, z);
}

abacus_half2 ABACUS_API __abacus_mad(abacus_half2 x, abacus_half2 y,
                                     abacus_half2 z) {
  return mad_helper(x, y, z);
}

abacus_half3 ABACUS_API __abacus_mad(abacus_half3 x, abacus_half3 y,
                                     abacus_half3 z) {
  return mad_helper(x, y, z);
}

abacus_half4 ABACUS_API __abacus_mad(abacus_half4 x, abacus_half4 y,
                                     abacus_half4 z) {
  return mad_helper(x, y, z);
}

abacus_half8 ABACUS_API __abacus_mad(abacus_half8 x, abacus_half8 y,
                                     abacus_half8 z) {
  return mad_helper(x, y, z);
}

abacus_half16 ABACUS_API __abacus_mad(abacus_half16 x, abacus_half16 y,
                                      abacus_half16 z) {
  return mad_helper(x, y, z);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_mad(abacus_float x, abacus_float y,
                                     abacus_float z) {
  return mad_helper(x, y, z);
}

abacus_float2 ABACUS_API __abacus_mad(abacus_float2 x, abacus_float2 y,
                                      abacus_float2 z) {
  return mad_helper(x, y, z);
}

abacus_float3 ABACUS_API __abacus_mad(abacus_float3 x, abacus_float3 y,
                                      abacus_float3 z) {
  return mad_helper(x, y, z);
}

abacus_float4 ABACUS_API __abacus_mad(abacus_float4 x, abacus_float4 y,
                                      abacus_float4 z) {
  return mad_helper(x, y, z);
}

abacus_float8 ABACUS_API __abacus_mad(abacus_float8 x, abacus_float8 y,
                                      abacus_float8 z) {
  return mad_helper(x, y, z);
}

abacus_float16 ABACUS_API __abacus_mad(abacus_float16 x, abacus_float16 y,
                                       abacus_float16 z) {
  return mad_helper(x, y, z);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_mad(abacus_double x, abacus_double y,
                                      abacus_double z) {
  return mad_helper(x, y, z);
}

abacus_double2 ABACUS_API __abacus_mad(abacus_double2 x, abacus_double2 y,
                                       abacus_double2 z) {
  return mad_helper(x, y, z);
}

abacus_double3 ABACUS_API __abacus_mad(abacus_double3 x, abacus_double3 y,
                                       abacus_double3 z) {
  return mad_helper(x, y, z);
}

abacus_double4 ABACUS_API __abacus_mad(abacus_double4 x, abacus_double4 y,
                                       abacus_double4 z) {
  return mad_helper(x, y, z);
}

abacus_double8 ABACUS_API __abacus_mad(abacus_double8 x, abacus_double8 y,
                                       abacus_double8 z) {
  return mad_helper(x, y, z);
}

abacus_double16 ABACUS_API __abacus_mad(abacus_double16 x, abacus_double16 y,
                                        abacus_double16 z) {
  return mad_helper(x, y, z);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
