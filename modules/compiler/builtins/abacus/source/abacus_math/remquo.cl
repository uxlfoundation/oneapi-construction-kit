// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_remquo(abacus_half x, abacus_half y,
                                         __private abacus_int* out_quotient);
abacus_half2 ABACUS_API __abacus_remquo(abacus_half2 x, abacus_half2 y,
                                          __private abacus_int2* out_quotient);
abacus_half3 ABACUS_API __abacus_remquo(abacus_half3 x, abacus_half3 y,
                                          __private abacus_int3* out_quotient);
abacus_half4 ABACUS_API __abacus_remquo(abacus_half4 x, abacus_half4 y,
                                          __private abacus_int4* out_quotient);
abacus_half8 ABACUS_API __abacus_remquo(abacus_half8 x, abacus_half8 y,
                                          __private abacus_int8* out_quotient);
abacus_half16 ABACUS_API __abacus_remquo(
    abacus_half16 x, abacus_half16 y, __private abacus_int16* out_quotient);

abacus_half ABACUS_API __abacus_remquo(abacus_half x, abacus_half y,
                                       __global abacus_int* out_quotient) {
  int quotient = 0;
  const half result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_half ABACUS_API __abacus_remquo(abacus_half x, abacus_half y,
                                       __local abacus_int* out_quotient) {
  int quotient = 0;
  const half result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_half2 ABACUS_API __abacus_remquo(abacus_half2 x, abacus_half2 y,
                                        __global abacus_int2* out_quotient) {
  int2 quotient = 0;
  const half2 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_half2 ABACUS_API __abacus_remquo(abacus_half2 x, abacus_half2 y,
                                        __local abacus_int2* out_quotient) {
  int2 quotient = 0;
  const half2 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_half3 ABACUS_API __abacus_remquo(abacus_half3 x, abacus_half3 y,
                                        __global abacus_int3* out_quotient) {
  int3 quotient = 0;
  const half3 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_half3 ABACUS_API __abacus_remquo(abacus_half3 x, abacus_half3 y,
                                        __local abacus_int3* out_quotient) {
  int3 quotient = 0;
  const half3 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_half4 ABACUS_API __abacus_remquo(abacus_half4 x, abacus_half4 y,
                                        __global abacus_int4* out_quotient) {
  int4 quotient = 0;
  const half4 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_half4 ABACUS_API __abacus_remquo(abacus_half4 x, abacus_half4 y,
                                        __local abacus_int4* out_quotient) {
  int4 quotient = 0;
  const half4 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_half8 ABACUS_API __abacus_remquo(abacus_half8 x, abacus_half8 y,
                                        __global abacus_int8* out_quotient) {
  int8 quotient = 0;
  const half8 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_half8 ABACUS_API __abacus_remquo(abacus_half8 x, abacus_half8 y,
                                        __local abacus_int8* out_quotient) {
  int8 quotient = 0;
  const half8 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_half16 ABACUS_API __abacus_remquo(
    abacus_half16 x, abacus_half16 y, __global abacus_int16* out_quotient) {
  int16 quotient = 0;
  const half16 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_half16 ABACUS_API __abacus_remquo(abacus_half16 x, abacus_half16 y,
                                         __local abacus_int16* out_quotient) {
  int16 quotient = 0;
  const half16 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_remquo(abacus_float x, abacus_float y,
                                        __private abacus_int* out_quotient);
abacus_float2 ABACUS_API __abacus_remquo(abacus_float2 x, abacus_float2 y,
                                         __private abacus_int2* out_quotient);
abacus_float3 ABACUS_API __abacus_remquo(abacus_float3 x, abacus_float3 y,
                                         __private abacus_int3* out_quotient);
abacus_float4 ABACUS_API __abacus_remquo(abacus_float4 x, abacus_float4 y,
                                         __private abacus_int4* out_quotient);
abacus_float8 ABACUS_API __abacus_remquo(abacus_float8 x, abacus_float8 y,
                                         __private abacus_int8* out_quotient);
abacus_float16 ABACUS_API __abacus_remquo(abacus_float16 x, abacus_float16 y,
                                          __private abacus_int16* out_quotient);

abacus_float ABACUS_API __abacus_remquo(abacus_float x, abacus_float y,
                                        __global abacus_int* out_quotient) {
  int quotient = 0;
  const float result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_float ABACUS_API __abacus_remquo(abacus_float x, abacus_float y,
                                        __local abacus_int* out_quotient) {
  int quotient = 0;
  const float result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_float2 ABACUS_API __abacus_remquo(abacus_float2 x, abacus_float2 y,
                                         __global abacus_int2* out_quotient) {
  int2 quotient = 0;
  const float2 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_float2 ABACUS_API __abacus_remquo(abacus_float2 x, abacus_float2 y,
                                         __local abacus_int2* out_quotient) {
  int2 quotient = 0;
  const float2 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_float3 ABACUS_API __abacus_remquo(abacus_float3 x, abacus_float3 y,
                                         __global abacus_int3* out_quotient) {
  int3 quotient = 0;
  const float3 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_float3 ABACUS_API __abacus_remquo(abacus_float3 x, abacus_float3 y,
                                         __local abacus_int3* out_quotient) {
  int3 quotient = 0;
  const float3 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_float4 ABACUS_API __abacus_remquo(abacus_float4 x, abacus_float4 y,
                                         __global abacus_int4* out_quotient) {
  int4 quotient = 0;
  const float4 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_float4 ABACUS_API __abacus_remquo(abacus_float4 x, abacus_float4 y,
                                         __local abacus_int4* out_quotient) {
  int4 quotient = 0;
  const float4 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_float8 ABACUS_API __abacus_remquo(abacus_float8 x, abacus_float8 y,
                                         __global abacus_int8* out_quotient) {
  int8 quotient = 0;
  const float8 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_float8 ABACUS_API __abacus_remquo(abacus_float8 x, abacus_float8 y,
                                         __local abacus_int8* out_quotient) {
  int8 quotient = 0;
  const float8 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_float16 ABACUS_API __abacus_remquo(abacus_float16 x, abacus_float16 y,
                                          __global abacus_int16* out_quotient) {
  int16 quotient = 0;
  const float16 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_float16 ABACUS_API __abacus_remquo(abacus_float16 x, abacus_float16 y,
                                          __local abacus_int16* out_quotient) {
  int16 quotient = 0;
  const float16 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_remquo(abacus_double x, abacus_double y,
                                         __private abacus_int* out_quotient);
abacus_double2 ABACUS_API __abacus_remquo(abacus_double2 x, abacus_double2 y,
                                          __private abacus_int2* out_quotient);
abacus_double3 ABACUS_API __abacus_remquo(abacus_double3 x, abacus_double3 y,
                                          __private abacus_int3* out_quotient);
abacus_double4 ABACUS_API __abacus_remquo(abacus_double4 x, abacus_double4 y,
                                          __private abacus_int4* out_quotient);
abacus_double8 ABACUS_API __abacus_remquo(abacus_double8 x, abacus_double8 y,
                                          __private abacus_int8* out_quotient);
abacus_double16 ABACUS_API __abacus_remquo(
    abacus_double16 x, abacus_double16 y, __private abacus_int16* out_quotient);

abacus_double ABACUS_API __abacus_remquo(abacus_double x, abacus_double y,
                                         __global abacus_int* out_quotient) {
  int quotient = 0;
  const double result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_double ABACUS_API __abacus_remquo(abacus_double x, abacus_double y,
                                         __local abacus_int* out_quotient) {
  int quotient = 0;
  const double result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_double2 ABACUS_API __abacus_remquo(abacus_double2 x, abacus_double2 y,
                                          __global abacus_int2* out_quotient) {
  int2 quotient = 0;
  const double2 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_double2 ABACUS_API __abacus_remquo(abacus_double2 x, abacus_double2 y,
                                          __local abacus_int2* out_quotient) {
  int2 quotient = 0;
  const double2 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_double3 ABACUS_API __abacus_remquo(abacus_double3 x, abacus_double3 y,
                                          __global abacus_int3* out_quotient) {
  int3 quotient = 0;
  const double3 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_double3 ABACUS_API __abacus_remquo(abacus_double3 x, abacus_double3 y,
                                          __local abacus_int3* out_quotient) {
  int3 quotient = 0;
  const double3 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_double4 ABACUS_API __abacus_remquo(abacus_double4 x, abacus_double4 y,
                                          __global abacus_int4* out_quotient) {
  int4 quotient = 0;
  const double4 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_double4 ABACUS_API __abacus_remquo(abacus_double4 x, abacus_double4 y,
                                          __local abacus_int4* out_quotient) {
  int4 quotient = 0;
  const double4 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_double8 ABACUS_API __abacus_remquo(abacus_double8 x, abacus_double8 y,
                                          __global abacus_int8* out_quotient) {
  int8 quotient = 0;
  const double8 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_double8 ABACUS_API __abacus_remquo(abacus_double8 x, abacus_double8 y,
                                          __local abacus_int8* out_quotient) {
  int8 quotient = 0;
  const double8 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_double16 ABACUS_API __abacus_remquo(
    abacus_double16 x, abacus_double16 y, __global abacus_int16* out_quotient) {
  int16 quotient = 0;
  const double16 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}

abacus_double16 ABACUS_API __abacus_remquo(abacus_double16 x, abacus_double16 y,
                                           __local abacus_int16* out_quotient) {
  int16 quotient = 0;
  const double16 result = __abacus_remquo(x, y, &quotient);
  *out_quotient = quotient;
  return result;
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
