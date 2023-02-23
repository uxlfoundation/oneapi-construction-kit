// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_lgamma_r(abacus_half x,
                                         __private abacus_int* out_sign);
abacus_half2 ABACUS_API __abacus_lgamma_r(abacus_half2 x,
                                          __private abacus_int2* out_sign);
abacus_half3 ABACUS_API __abacus_lgamma_r(abacus_half3 x,
                                          __private abacus_int3* out_sign);
abacus_half4 ABACUS_API __abacus_lgamma_r(abacus_half4 x,
                                          __private abacus_int4* out_sign);
abacus_half8 ABACUS_API __abacus_lgamma_r(abacus_half8 x,
                                          __private abacus_int8* out_sign);
abacus_half16 ABACUS_API __abacus_lgamma_r(abacus_half16 x,
                                           __private abacus_int16* out_sign);

abacus_half ABACUS_API __abacus_lgamma_r(abacus_half x,
                                         __global abacus_int* out_sign) {
  abacus_int sign = 0;
  const abacus_half result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_half ABACUS_API __abacus_lgamma_r(abacus_half x,
                                         __local abacus_int* out_sign) {
  abacus_int sign = 0;
  const abacus_half result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_half2 ABACUS_API __abacus_lgamma_r(abacus_half2 x,
                                          __global abacus_int2* out_sign) {
  abacus_int2 sign = 0;
  const abacus_half2 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_half2 ABACUS_API __abacus_lgamma_r(abacus_half2 x,
                                          __local abacus_int2* out_sign) {
  abacus_int2 sign = 0;
  const abacus_half2 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_half3 ABACUS_API __abacus_lgamma_r(abacus_half3 x,
                                          __global abacus_int3* out_sign) {
  abacus_int3 sign = 0;
  const abacus_half3 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_half3 ABACUS_API __abacus_lgamma_r(abacus_half3 x,
                                          __local abacus_int3* out_sign) {
  abacus_int3 sign = 0;
  const abacus_half3 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_half4 ABACUS_API __abacus_lgamma_r(abacus_half4 x,
                                          __global abacus_int4* out_sign) {
  abacus_int4 sign = 0;
  const abacus_half4 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_half4 ABACUS_API __abacus_lgamma_r(abacus_half4 x,
                                          __local abacus_int4* out_sign) {
  abacus_int4 sign = 0;
  const abacus_half4 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_half8 ABACUS_API __abacus_lgamma_r(abacus_half8 x,
                                          __global abacus_int8* out_sign) {
  abacus_int8 sign = 0;
  const abacus_half8 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_half8 ABACUS_API __abacus_lgamma_r(abacus_half8 x,
                                          __local abacus_int8* out_sign) {
  abacus_int8 sign = 0;
  const abacus_half8 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_half16 ABACUS_API __abacus_lgamma_r(abacus_half16 x,
                                           __global abacus_int16* out_sign) {
  abacus_int16 sign = 0;
  const abacus_half16 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_half16 ABACUS_API __abacus_lgamma_r(abacus_half16 x,
                                           __local abacus_int16* out_sign) {
  abacus_int16 sign = 0;
  const abacus_half16 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_lgamma_r(abacus_float x,
                                          __private abacus_int* out_sign);
abacus_float2 ABACUS_API __abacus_lgamma_r(abacus_float2 x,
                                           __private abacus_int2* out_sign);
abacus_float3 ABACUS_API __abacus_lgamma_r(abacus_float3 x,
                                           __private abacus_int3* out_sign);
abacus_float4 ABACUS_API __abacus_lgamma_r(abacus_float4 x,
                                           __private abacus_int4* out_sign);
abacus_float8 ABACUS_API __abacus_lgamma_r(abacus_float8 x,
                                           __private abacus_int8* out_sign);
abacus_float16 ABACUS_API __abacus_lgamma_r(abacus_float16 x,
                                            __private abacus_int16* out_sign);

abacus_float ABACUS_API __abacus_lgamma_r(abacus_float x,
                                          __global abacus_int* out_sign) {
  abacus_int sign = 0;
  const abacus_float result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_float ABACUS_API __abacus_lgamma_r(abacus_float x,
                                          __local abacus_int* out_sign) {
  abacus_int sign = 0;
  const abacus_float result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_float2 ABACUS_API __abacus_lgamma_r(abacus_float2 x,
                                           __global abacus_int2* out_sign) {
  abacus_int2 sign = 0;
  const abacus_float2 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_float2 ABACUS_API __abacus_lgamma_r(abacus_float2 x,
                                           __local abacus_int2* out_sign) {
  abacus_int2 sign = 0;
  const abacus_float2 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_float3 ABACUS_API __abacus_lgamma_r(abacus_float3 x,
                                           __global abacus_int3* out_sign) {
  abacus_int3 sign = 0;
  const abacus_float3 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_float3 ABACUS_API __abacus_lgamma_r(abacus_float3 x,
                                           __local abacus_int3* out_sign) {
  abacus_int3 sign = 0;
  const abacus_float3 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_float4 ABACUS_API __abacus_lgamma_r(abacus_float4 x,
                                           __global abacus_int4* out_sign) {
  abacus_int4 sign = 0;
  const abacus_float4 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_float4 ABACUS_API __abacus_lgamma_r(abacus_float4 x,
                                           __local abacus_int4* out_sign) {
  abacus_int4 sign = 0;
  const abacus_float4 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_float8 ABACUS_API __abacus_lgamma_r(abacus_float8 x,
                                           __global abacus_int8* out_sign) {
  abacus_int8 sign = 0;
  const abacus_float8 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_float8 ABACUS_API __abacus_lgamma_r(abacus_float8 x,
                                           __local abacus_int8* out_sign) {
  abacus_int8 sign = 0;
  const abacus_float8 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_float16 ABACUS_API __abacus_lgamma_r(abacus_float16 x,
                                            __global abacus_int16* out_sign) {
  abacus_int16 sign = 0;
  const abacus_float16 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_float16 ABACUS_API __abacus_lgamma_r(abacus_float16 x,
                                            __local abacus_int16* out_sign) {
  abacus_int16 sign = 0;
  const abacus_float16 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_lgamma_r(abacus_double x,
                                           __private abacus_int* out_sign);
abacus_double2 ABACUS_API __abacus_lgamma_r(abacus_double2 x,
                                            __private abacus_int2* out_sign);
abacus_double3 ABACUS_API __abacus_lgamma_r(abacus_double3 x,
                                            __private abacus_int3* out_sign);
abacus_double4 ABACUS_API __abacus_lgamma_r(abacus_double4 x,
                                            __private abacus_int4* out_sign);
abacus_double8 ABACUS_API __abacus_lgamma_r(abacus_double8 x,
                                            __private abacus_int8* out_sign);
abacus_double16 ABACUS_API __abacus_lgamma_r(abacus_double16 x,
                                             __private abacus_int16* out_sign);

abacus_double ABACUS_API __abacus_lgamma_r(abacus_double x,
                                           __global abacus_int* out_sign) {
  abacus_int sign = 0;
  const abacus_double result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_double ABACUS_API __abacus_lgamma_r(abacus_double x,
                                           __local abacus_int* out_sign) {
  abacus_int sign = 0;
  const abacus_double result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_double2 ABACUS_API __abacus_lgamma_r(abacus_double2 x,
                                            __global abacus_int2* out_sign) {
  abacus_int2 sign = 0;
  const abacus_double2 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_double2 ABACUS_API __abacus_lgamma_r(abacus_double2 x,
                                            __local abacus_int2* out_sign) {
  abacus_int2 sign = 0;
  const abacus_double2 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_double3 ABACUS_API __abacus_lgamma_r(abacus_double3 x,
                                            __global abacus_int3* out_sign) {
  abacus_int3 sign = 0;
  const abacus_double3 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_double3 ABACUS_API __abacus_lgamma_r(abacus_double3 x,
                                            __local abacus_int3* out_sign) {
  abacus_int3 sign = 0;
  const abacus_double3 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_double4 ABACUS_API __abacus_lgamma_r(abacus_double4 x,
                                            __global abacus_int4* out_sign) {
  abacus_int4 sign = 0;
  const abacus_double4 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_double4 ABACUS_API __abacus_lgamma_r(abacus_double4 x,
                                            __local abacus_int4* out_sign) {
  abacus_int4 sign = 0;
  const abacus_double4 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_double8 ABACUS_API __abacus_lgamma_r(abacus_double8 x,
                                            __global abacus_int8* out_sign) {
  abacus_int8 sign = 0;
  const abacus_double8 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_double8 ABACUS_API __abacus_lgamma_r(abacus_double8 x,
                                            __local abacus_int8* out_sign) {
  abacus_int8 sign = 0;
  const abacus_double8 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_double16 ABACUS_API __abacus_lgamma_r(abacus_double16 x,
                                             __global abacus_int16* out_sign) {
  abacus_int16 sign = 0;
  const abacus_double16 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}

abacus_double16 ABACUS_API __abacus_lgamma_r(abacus_double16 x,
                                             __local abacus_int16* out_sign) {
  abacus_int16 sign = 0;
  const abacus_double16 result = __abacus_lgamma_r(x, &sign);
  *out_sign = sign;
  return result;
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
