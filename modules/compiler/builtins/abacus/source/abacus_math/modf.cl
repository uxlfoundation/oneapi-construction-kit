// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API
__abacus_modf(abacus_half x, __private abacus_half* out_whole_number);
abacus_half2 ABACUS_API
__abacus_modf(abacus_half2 x, __private abacus_half2* out_whole_number);
abacus_half3 ABACUS_API
__abacus_modf(abacus_half3 x, __private abacus_half3* out_whole_number);
abacus_half4 ABACUS_API
__abacus_modf(abacus_half4 x, __private abacus_half4* out_whole_number);
abacus_half8 ABACUS_API
__abacus_modf(abacus_half8 x, __private abacus_half8* out_whole_number);
abacus_half16 ABACUS_API
__abacus_modf(abacus_half16 x, __private abacus_half16* out_whole_number);

abacus_half ABACUS_API
__abacus_modf(abacus_half x, __global abacus_half* out_whole_number) {
  abacus_half whole_number = 0.0f16;
  abacus_half result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_half ABACUS_API
__abacus_modf(abacus_half x, __local abacus_half* out_whole_number) {
  abacus_half whole_number = 0.0f16;
  abacus_half result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_half2 ABACUS_API
__abacus_modf(abacus_half2 x, __global abacus_half2* out_whole_number) {
  abacus_half2 whole_number = 0.0f16;
  abacus_half2 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_half2 ABACUS_API
__abacus_modf(abacus_half2 x, __local abacus_half2* out_whole_number) {
  abacus_half2 whole_number = 0.0f16;
  abacus_half2 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_half3 ABACUS_API
__abacus_modf(abacus_half3 x, __global abacus_half3* out_whole_number) {
  abacus_half3 whole_number = 0.0f16;
  abacus_half3 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_half3 ABACUS_API
__abacus_modf(abacus_half3 x, __local abacus_half3* out_whole_number) {
  abacus_half3 whole_number = 0.0f16;
  abacus_half3 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_half4 ABACUS_API
__abacus_modf(abacus_half4 x, __global abacus_half4* out_whole_number) {
  abacus_half4 whole_number = 0.0f16;
  abacus_half4 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_half4 ABACUS_API
__abacus_modf(abacus_half4 x, __local abacus_half4* out_whole_number) {
  abacus_half4 whole_number = 0.0f16;
  abacus_half4 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_half8 ABACUS_API
__abacus_modf(abacus_half8 x, __global abacus_half8* out_whole_number) {
  abacus_half8 whole_number = 0.0f16;
  abacus_half8 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_half8 ABACUS_API
__abacus_modf(abacus_half8 x, __local abacus_half8* out_whole_number) {
  abacus_half8 whole_number = 0.0f16;
  abacus_half8 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_half16 ABACUS_API
__abacus_modf(abacus_half16 x, __global abacus_half16* out_whole_number) {
  abacus_half16 whole_number = 0.0f16;
  abacus_half16 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_half16 ABACUS_API
__abacus_modf(abacus_half16 x, __local abacus_half16* out_whole_number) {
  abacus_half16 whole_number = 0.0f16;
  abacus_half16 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_modf(abacus_float x,
                                      __private abacus_float* out_whole_number);
abacus_float2 ABACUS_API
__abacus_modf(abacus_float2 x, __private abacus_float2* out_whole_number);
abacus_float3 ABACUS_API
__abacus_modf(abacus_float3 x, __private abacus_float3* out_whole_number);
abacus_float4 ABACUS_API
__abacus_modf(abacus_float4 x, __private abacus_float4* out_whole_number);
abacus_float8 ABACUS_API
__abacus_modf(abacus_float8 x, __private abacus_float8* out_whole_number);
abacus_float16 ABACUS_API
__abacus_modf(abacus_float16 x, __private abacus_float16* out_whole_number);

abacus_float ABACUS_API __abacus_modf(abacus_float x,
                                      __global abacus_float* out_whole_number) {
  abacus_float whole_number = 0.0f;
  abacus_float result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_float ABACUS_API __abacus_modf(abacus_float x,
                                      __local abacus_float* out_whole_number) {
  abacus_float whole_number = 0.0f;
  abacus_float result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_float2 ABACUS_API
__abacus_modf(abacus_float2 x, __global abacus_float2* out_whole_number) {
  abacus_float2 whole_number = 0.0f;
  abacus_float2 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_float2 ABACUS_API
__abacus_modf(abacus_float2 x, __local abacus_float2* out_whole_number) {
  abacus_float2 whole_number = 0.0f;
  abacus_float2 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_float3 ABACUS_API
__abacus_modf(abacus_float3 x, __global abacus_float3* out_whole_number) {
  abacus_float3 whole_number = 0.0f;
  abacus_float3 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_float3 ABACUS_API
__abacus_modf(abacus_float3 x, __local abacus_float3* out_whole_number) {
  abacus_float3 whole_number = 0.0f;
  abacus_float3 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_float4 ABACUS_API
__abacus_modf(abacus_float4 x, __global abacus_float4* out_whole_number) {
  abacus_float4 whole_number = 0.0f;
  abacus_float4 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_float4 ABACUS_API
__abacus_modf(abacus_float4 x, __local abacus_float4* out_whole_number) {
  abacus_float4 whole_number = 0.0f;
  abacus_float4 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_float8 ABACUS_API
__abacus_modf(abacus_float8 x, __global abacus_float8* out_whole_number) {
  abacus_float8 whole_number = 0.0f;
  abacus_float8 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_float8 ABACUS_API
__abacus_modf(abacus_float8 x, __local abacus_float8* out_whole_number) {
  abacus_float8 whole_number = 0.0f;
  abacus_float8 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_float16 ABACUS_API
__abacus_modf(abacus_float16 x, __global abacus_float16* out_whole_number) {
  abacus_float16 whole_number = 0.0f;
  abacus_float16 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_float16 ABACUS_API
__abacus_modf(abacus_float16 x, __local abacus_float16* out_whole_number) {
  abacus_float16 whole_number = 0.0f;
  abacus_float16 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API
__abacus_modf(abacus_double x, __private abacus_double* out_whole_number);
abacus_double2 ABACUS_API
__abacus_modf(abacus_double2 x, __private abacus_double2* out_whole_number);
abacus_double3 ABACUS_API
__abacus_modf(abacus_double3 x, __private abacus_double3* out_whole_number);
abacus_double4 ABACUS_API
__abacus_modf(abacus_double4 x, __private abacus_double4* out_whole_number);
abacus_double8 ABACUS_API
__abacus_modf(abacus_double8 x, __private abacus_double8* out_whole_number);
abacus_double16 ABACUS_API
__abacus_modf(abacus_double16 x, __private abacus_double16* out_whole_number);

abacus_double ABACUS_API
__abacus_modf(abacus_double x, __global abacus_double* out_whole_number) {
  abacus_double whole_number = 0.0;
  abacus_double result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_double ABACUS_API
__abacus_modf(abacus_double x, __local abacus_double* out_whole_number) {
  abacus_double whole_number = 0.0;
  abacus_double result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_double2 ABACUS_API
__abacus_modf(abacus_double2 x, __global abacus_double2* out_whole_number) {
  abacus_double2 whole_number = 0.0;
  abacus_double2 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_double2 ABACUS_API
__abacus_modf(abacus_double2 x, __local abacus_double2* out_whole_number) {
  abacus_double2 whole_number = 0.0;
  abacus_double2 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_double3 ABACUS_API
__abacus_modf(abacus_double3 x, __global abacus_double3* out_whole_number) {
  abacus_double3 whole_number = 0.0;
  abacus_double3 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_double3 ABACUS_API
__abacus_modf(abacus_double3 x, __local abacus_double3* out_whole_number) {
  abacus_double3 whole_number = 0.0;
  abacus_double3 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_double4 ABACUS_API
__abacus_modf(abacus_double4 x, __global abacus_double4* out_whole_number) {
  abacus_double4 whole_number = 0.0;
  abacus_double4 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_double4 ABACUS_API
__abacus_modf(abacus_double4 x, __local abacus_double4* out_whole_number) {
  abacus_double4 whole_number = 0.0;
  abacus_double4 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_double8 ABACUS_API
__abacus_modf(abacus_double8 x, __global abacus_double8* out_whole_number) {
  abacus_double8 whole_number = 0.0;
  abacus_double8 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_double8 ABACUS_API
__abacus_modf(abacus_double8 x, __local abacus_double8* out_whole_number) {
  abacus_double8 whole_number = 0.0;
  abacus_double8 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_double16 ABACUS_API
__abacus_modf(abacus_double16 x, __global abacus_double16* out_whole_number) {
  abacus_double16 whole_number = 0.0;
  abacus_double16 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}

abacus_double16 ABACUS_API
__abacus_modf(abacus_double16 x, __local abacus_double16* out_whole_number) {
  abacus_double16 whole_number = 0.0;
  abacus_double16 result = __abacus_modf(x, &whole_number);
  *out_whole_number = whole_number;
  return result;
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
