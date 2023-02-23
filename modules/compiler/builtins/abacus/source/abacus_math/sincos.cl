// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_sincos(abacus_half x,
                                       __private abacus_half* out_cos);
abacus_half2 ABACUS_API __abacus_sincos(abacus_half2 x,
                                        __private abacus_half2* out_cos);
abacus_half3 ABACUS_API __abacus_sincos(abacus_half3 x,
                                        __private abacus_half3* out_cos);
abacus_half4 ABACUS_API __abacus_sincos(abacus_half4 x,
                                        __private abacus_half4* out_cos);
abacus_half8 ABACUS_API __abacus_sincos(abacus_half8 x,
                                        __private abacus_half8* out_cos);
abacus_half16 ABACUS_API __abacus_sincos(abacus_half16 x,
                                         __private abacus_half16* out_cos);
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_sincos(abacus_float x,
                                        __private abacus_float* out_cos);
abacus_float2 ABACUS_API __abacus_sincos(abacus_float2 x,
                                         __private abacus_float2* out_cos);
abacus_float3 ABACUS_API __abacus_sincos(abacus_float3 x,
                                         __private abacus_float3* out_cos);
abacus_float4 ABACUS_API __abacus_sincos(abacus_float4 x,
                                         __private abacus_float4* out_cos);
abacus_float8 ABACUS_API __abacus_sincos(abacus_float8 x,
                                         __private abacus_float8* out_cos);
abacus_float16 ABACUS_API __abacus_sincos(abacus_float16 x,
                                          __private abacus_float16* out_cos);

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_sincos(abacus_double x,
                                         __private abacus_double* out_cos);
abacus_double2 ABACUS_API __abacus_sincos(abacus_double2 x,
                                          __private abacus_double2* out_cos);
abacus_double3 ABACUS_API __abacus_sincos(abacus_double3 x,
                                          __private abacus_double3* out_cos);
abacus_double4 ABACUS_API __abacus_sincos(abacus_double4 x,
                                          __private abacus_double4* out_cos);
abacus_double8 ABACUS_API __abacus_sincos(abacus_double8 x,
                                          __private abacus_double8* out_cos);
abacus_double16 ABACUS_API __abacus_sincos(abacus_double16 x,
                                           __private abacus_double16* out_cos);
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_sincos(abacus_half x,
                                       __global abacus_half* out_cos) {
  abacus_half cosValue = 0.0f16;

  const abacus_half result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_half ABACUS_API __abacus_sincos(abacus_half x,
                                       __local abacus_half* out_cos) {
  abacus_half cosValue = 0.0f16;

  const abacus_half result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_half2 ABACUS_API __abacus_sincos(abacus_half2 x,
                                        __global abacus_half2* out_cos) {
  abacus_half2 cosValue = 0.0f16;

  const abacus_half2 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_half2 ABACUS_API __abacus_sincos(abacus_half2 x,
                                        __local abacus_half2* out_cos) {
  abacus_half2 cosValue = 0.0f16;

  const abacus_half2 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_half3 ABACUS_API __abacus_sincos(abacus_half3 x,
                                        __global abacus_half3* out_cos) {
  abacus_half3 cosValue = 0.0f16;

  const abacus_half3 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_half3 ABACUS_API __abacus_sincos(abacus_half3 x,
                                        __local abacus_half3* out_cos) {
  abacus_half3 cosValue = 0.0f16;

  const abacus_half3 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_half4 ABACUS_API __abacus_sincos(abacus_half4 x,
                                        __global abacus_half4* out_cos) {
  abacus_half4 cosValue = 0.0f16;

  const abacus_half4 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_half4 ABACUS_API __abacus_sincos(abacus_half4 x,
                                        __local abacus_half4* out_cos) {
  abacus_half4 cosValue = 0.0f16;

  const abacus_half4 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_half8 ABACUS_API __abacus_sincos(abacus_half8 x,
                                        __global abacus_half8* out_cos) {
  abacus_half8 cosValue = 0.0f16;

  const abacus_half8 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_half8 ABACUS_API __abacus_sincos(abacus_half8 x,
                                        __local abacus_half8* out_cos) {
  abacus_half8 cosValue = 0.0f16;

  const abacus_half8 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_half16 ABACUS_API __abacus_sincos(abacus_half16 x,
                                         __global abacus_half16* out_cos) {
  abacus_half16 cosValue = 0.0f16;

  const abacus_half16 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_half16 ABACUS_API __abacus_sincos(abacus_half16 x,
                                         __local abacus_half16* out_cos) {
  abacus_half16 cosValue = 0.0f16;

  const abacus_half16 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_sincos(abacus_float x,
                                        __global abacus_float* out_cos) {
  abacus_float cosValue = 0.0f;

  const abacus_float result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_float ABACUS_API __abacus_sincos(abacus_float x,
                                        __local abacus_float* out_cos) {
  abacus_float cosValue = 0.0f;

  const abacus_float result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_float2 ABACUS_API __abacus_sincos(abacus_float2 x,
                                         __global abacus_float2* out_cos) {
  abacus_float2 cosValue = 0.0f;

  const abacus_float2 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_float2 ABACUS_API __abacus_sincos(abacus_float2 x,
                                         __local abacus_float2* out_cos) {
  abacus_float2 cosValue = 0.0f;

  const abacus_float2 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_float3 ABACUS_API __abacus_sincos(abacus_float3 x,
                                         __global abacus_float3* out_cos) {
  abacus_float3 cosValue = 0.0f;

  const abacus_float3 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_float3 ABACUS_API __abacus_sincos(abacus_float3 x,
                                         __local abacus_float3* out_cos) {
  abacus_float3 cosValue = 0.0f;

  const abacus_float3 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_float4 ABACUS_API __abacus_sincos(abacus_float4 x,
                                         __global abacus_float4* out_cos) {
  abacus_float4 cosValue = 0.0f;

  const abacus_float4 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_float4 ABACUS_API __abacus_sincos(abacus_float4 x,
                                         __local abacus_float4* out_cos) {
  abacus_float4 cosValue = 0.0f;

  const abacus_float4 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_float8 ABACUS_API __abacus_sincos(abacus_float8 x,
                                         __global abacus_float8* out_cos) {
  abacus_float8 cosValue = 0.0f;

  const abacus_float8 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_float8 ABACUS_API __abacus_sincos(abacus_float8 x,
                                         __local abacus_float8* out_cos) {
  abacus_float8 cosValue = 0.0f;

  const abacus_float8 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_float16 ABACUS_API __abacus_sincos(abacus_float16 x,
                                          __global abacus_float16* out_cos) {
  abacus_float16 cosValue = 0.0f;

  const abacus_float16 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_float16 ABACUS_API __abacus_sincos(abacus_float16 x,
                                          __local abacus_float16* out_cos) {
  abacus_float16 cosValue = 0.0f;

  const abacus_float16 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_sincos(abacus_double x,
                                         __global abacus_double* out_cos) {
  abacus_double cosValue = 0.0f;

  const abacus_double result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_double ABACUS_API __abacus_sincos(abacus_double x,
                                         __local abacus_double* out_cos) {
  abacus_double cosValue = 0.0f;

  const abacus_double result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_double2 ABACUS_API __abacus_sincos(abacus_double2 x,
                                          __global abacus_double2* out_cos) {
  abacus_double2 cosValue = 0.0f;

  const abacus_double2 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_double2 ABACUS_API __abacus_sincos(abacus_double2 x,
                                          __local abacus_double2* out_cos) {
  abacus_double2 cosValue = 0.0f;

  const abacus_double2 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_double3 ABACUS_API __abacus_sincos(abacus_double3 x,
                                          __global abacus_double3* out_cos) {
  abacus_double3 cosValue = 0.0f;

  const abacus_double3 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_double3 ABACUS_API __abacus_sincos(abacus_double3 x,
                                          __local abacus_double3* out_cos) {
  abacus_double3 cosValue = 0.0f;

  const abacus_double3 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_double4 ABACUS_API __abacus_sincos(abacus_double4 x,
                                          __global abacus_double4* out_cos) {
  abacus_double4 cosValue = 0.0f;

  const abacus_double4 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_double4 ABACUS_API __abacus_sincos(abacus_double4 x,
                                          __local abacus_double4* out_cos) {
  abacus_double4 cosValue = 0.0f;

  const abacus_double4 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_double8 ABACUS_API __abacus_sincos(abacus_double8 x,
                                          __global abacus_double8* out_cos) {
  abacus_double8 cosValue = 0.0f;

  const abacus_double8 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_double8 ABACUS_API __abacus_sincos(abacus_double8 x,
                                          __local abacus_double8* out_cos) {
  abacus_double8 cosValue = 0.0f;

  const abacus_double8 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_double16 ABACUS_API __abacus_sincos(abacus_double16 x,
                                           __global abacus_double16* out_cos) {
  abacus_double16 cosValue = 0.0f;

  const abacus_double16 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}

abacus_double16 ABACUS_API __abacus_sincos(abacus_double16 x,
                                           __local abacus_double16* out_cos) {
  abacus_double16 cosValue = 0.0f;

  const abacus_double16 result = __abacus_sincos(x, &cosValue);

  *out_cos = cosValue;

  return result;
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
