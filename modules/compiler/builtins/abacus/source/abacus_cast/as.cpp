// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <abacus/abacus_cast.h>
#include <abacus/abacus_detail_cast.h>

#define DEF(IN_TYPE, OUT_TYPE)                                              \
  abacus_##OUT_TYPE ABACUS_API __abacus_as_##OUT_TYPE(abacus_##IN_TYPE x) { \
    return abacus::detail::cast::as<abacus_##OUT_TYPE>(x);                  \
  }

#define DEF_BOTH_WAYS(TYPE, TYPE2) \
  DEF(TYPE, TYPE2)                 \
  DEF(TYPE2, TYPE)

DEF_BOTH_WAYS(char, uchar)
DEF(char, char)
DEF(uchar, uchar)

#ifdef __CA_BUILTINS_HALF_SUPPORT
#define DEF2(TYPE)  \
  DEF(TYPE, char2)  \
  DEF(TYPE, uchar2) \
  DEF(TYPE, short)  \
  DEF(TYPE, ushort) \
  DEF(TYPE, half)
#else
#define DEF2(TYPE)  \
  DEF(TYPE, char2)  \
  DEF(TYPE, uchar2) \
  DEF(TYPE, short)  \
  DEF(TYPE, ushort)
#endif

DEF2(char2)
DEF2(uchar2)
DEF2(short)
DEF2(ushort)
#ifdef __CA_BUILTINS_HALF_SUPPORT
DEF2(half)
#endif

#ifdef __CA_BUILTINS_HALF_SUPPORT
#define DEF3(TYPE)   \
  DEF(TYPE, char4)   \
  DEF(TYPE, uchar4)  \
  DEF(TYPE, short2)  \
  DEF(TYPE, ushort2) \
  DEF(TYPE, half2)   \
  DEF(TYPE, int)     \
  DEF(TYPE, uint)    \
  DEF(TYPE, float)
#else
#define DEF3(TYPE)   \
  DEF(TYPE, char4)   \
  DEF(TYPE, uchar4)  \
  DEF(TYPE, short2)  \
  DEF(TYPE, ushort2) \
  DEF(TYPE, int)     \
  DEF(TYPE, uint)    \
  DEF(TYPE, float)
#endif

DEF3(char4)
DEF3(uchar4)
DEF3(short2)
DEF3(ushort2)
#ifdef __CA_BUILTINS_HALF_SUPPORT
DEF3(half2)
#endif
DEF3(int)
DEF3(uint)
DEF3(float)

#ifdef __CA_BUILTINS_HALF_SUPPORT
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
#define DEF4(TYPE)   \
  DEF(TYPE, char8)   \
  DEF(TYPE, uchar8)  \
  DEF(TYPE, short4)  \
  DEF(TYPE, ushort4) \
  DEF(TYPE, half4)   \
  DEF(TYPE, int2)    \
  DEF(TYPE, uint2)   \
  DEF(TYPE, float2)  \
  DEF(TYPE, long)    \
  DEF(TYPE, ulong)   \
  DEF(TYPE, double)
#else // __CA_BUILTINS_DOUBLE_SUPPORT
#define DEF4(TYPE)   \
  DEF(TYPE, char8)   \
  DEF(TYPE, uchar8)  \
  DEF(TYPE, short4)  \
  DEF(TYPE, ushort4) \
  DEF(TYPE, half4)   \
  DEF(TYPE, int2)    \
  DEF(TYPE, uint2)   \
  DEF(TYPE, float2)  \
  DEF(TYPE, long)    \
  DEF(TYPE, ulong)
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
#else // __CA_BUILTINS_HALF_SUPPORT
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
#define DEF4(TYPE)   \
  DEF(TYPE, char8)   \
  DEF(TYPE, uchar8)  \
  DEF(TYPE, short4)  \
  DEF(TYPE, ushort4) \
  DEF(TYPE, int2)    \
  DEF(TYPE, uint2)   \
  DEF(TYPE, float2)  \
  DEF(TYPE, long)    \
  DEF(TYPE, ulong)   \
  DEF(TYPE, double)
#else // __CA_BUILTINS_DOUBLE_SUPPORT
#define DEF4(TYPE)   \
  DEF(TYPE, char8)   \
  DEF(TYPE, uchar8)  \
  DEF(TYPE, short4)  \
  DEF(TYPE, ushort4) \
  DEF(TYPE, int2)    \
  DEF(TYPE, uint2)   \
  DEF(TYPE, float2)  \
  DEF(TYPE, long)    \
  DEF(TYPE, ulong)
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
#endif // __CA_BUILTINS_HALF_SUPPORT

DEF4(char8)
DEF4(uchar8)
DEF4(short4)
DEF4(ushort4)
#ifdef __CA_BUILTINS_HALF_SUPPORT
DEF4(half4)
#endif
DEF4(int2)
DEF4(uint2)
DEF4(float2)
DEF4(long)
DEF4(ulong)
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF4(double)
#endif // __CA_BUILTINS_DOUBLE_SUPPORT

#ifdef __CA_BUILTINS_HALF_SUPPORT
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
#define DEF5(TYPE)   \
  DEF(TYPE, char16)  \
  DEF(TYPE, uchar16) \
  DEF(TYPE, short8)  \
  DEF(TYPE, ushort8) \
  DEF(TYPE, half8)   \
  DEF(TYPE, int4)    \
  DEF(TYPE, uint4)   \
  DEF(TYPE, float4)  \
  DEF(TYPE, long2)   \
  DEF(TYPE, ulong2)  \
  DEF(TYPE, double2)
#else // __CA_BUILTINS_DOUBLE_SUPPORT
#define DEF5(TYPE)   \
  DEF(TYPE, char16)  \
  DEF(TYPE, uchar16) \
  DEF(TYPE, short8)  \
  DEF(TYPE, ushort8) \
  DEF(TYPE, half8)   \
  DEF(TYPE, int4)    \
  DEF(TYPE, uint4)   \
  DEF(TYPE, float4)  \
  DEF(TYPE, long2)   \
  DEF(TYPE, ulong2)
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
#else // __CA_BUILTINS_HALF_SUPPORT
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
#define DEF5(TYPE)   \
  DEF(TYPE, char16)  \
  DEF(TYPE, uchar16) \
  DEF(TYPE, short8)  \
  DEF(TYPE, ushort8) \
  DEF(TYPE, int4)    \
  DEF(TYPE, uint4)   \
  DEF(TYPE, float4)  \
  DEF(TYPE, long2)   \
  DEF(TYPE, ulong2)  \
  DEF(TYPE, double2)
#else // __CA_BUILTINS_DOUBLE_SUPPORT
#define DEF5(TYPE)   \
  DEF(TYPE, char16)  \
  DEF(TYPE, uchar16) \
  DEF(TYPE, short8)  \
  DEF(TYPE, ushort8) \
  DEF(TYPE, int4)    \
  DEF(TYPE, uint4)   \
  DEF(TYPE, float4)  \
  DEF(TYPE, long2)   \
  DEF(TYPE, ulong2)
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
#endif // __CA_BUILTINS_HALF_SUPPORT

DEF5(char16)
DEF5(uchar16)
DEF5(short8)
DEF5(ushort8)
#ifdef __CA_BUILTINS_HALF_SUPPORT
DEF5(half8)
#endif
DEF5(int4)
DEF5(uint4)
DEF5(float4)
DEF5(long2)
DEF5(ulong2)
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF5(double2)
#endif // __CA_BUILTINS_DOUBLE_SUPPORT

#ifdef __CA_BUILTINS_HALF_SUPPORT
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
#define DEF6(TYPE)    \
  DEF(TYPE, short16)  \
  DEF(TYPE, ushort16) \
  DEF(TYPE, half16)   \
  DEF(TYPE, int8)     \
  DEF(TYPE, uint8)    \
  DEF(TYPE, float8)   \
  DEF(TYPE, long4)    \
  DEF(TYPE, ulong4)   \
  DEF(TYPE, double4)
#else // __CA_BUILTINS_DOUBLE_SUPPORT
#define DEF6(TYPE)    \
  DEF(TYPE, short16)  \
  DEF(TYPE, ushort16) \
  DEF(TYPE, half16)   \
  DEF(TYPE, int8)     \
  DEF(TYPE, uint8)    \
  DEF(TYPE, float8)   \
  DEF(TYPE, long4)    \
  DEF(TYPE, ulong4)
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
#else // __CA_BUILTINS_HALF_SUPPORT
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
#define DEF6(TYPE)    \
  DEF(TYPE, short16)  \
  DEF(TYPE, ushort16) \
  DEF(TYPE, int8)     \
  DEF(TYPE, uint8)    \
  DEF(TYPE, float8)   \
  DEF(TYPE, long4)    \
  DEF(TYPE, ulong4)   \
  DEF(TYPE, double4)
#else // __CA_BUILTINS_DOUBLE_SUPPORT
#define DEF6(TYPE)    \
  DEF(TYPE, short16)  \
  DEF(TYPE, ushort16) \
  DEF(TYPE, int8)     \
  DEF(TYPE, uint8)    \
  DEF(TYPE, float8)   \
  DEF(TYPE, long4)    \
  DEF(TYPE, ulong4)
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
#endif // __CA_BUILTINS_HALF_SUPPORT

DEF6(short16)
DEF6(ushort16)
#ifdef __CA_BUILTINS_HALF_SUPPORT
DEF6(half16)
#endif
DEF6(int8)
DEF6(uint8)
DEF6(float8)
DEF6(long4)
DEF6(ulong4)
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF6(double4)
#endif // __CA_BUILTINS_DOUBLE_SUPPORT

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
#define DEF7(TYPE)   \
  DEF(TYPE, int16)   \
  DEF(TYPE, uint16)  \
  DEF(TYPE, float16) \
  DEF(TYPE, long8)   \
  DEF(TYPE, ulong8)  \
  DEF(TYPE, double8)
#else
#define DEF7(TYPE)   \
  DEF(TYPE, int16)   \
  DEF(TYPE, uint16)  \
  DEF(TYPE, float16) \
  DEF(TYPE, long8)   \
  DEF(TYPE, ulong8)
#endif // __CA_BUILTINS_DOUBLE_SUPPORT

DEF7(int16)
DEF7(uint16)
DEF7(float16)
DEF7(long8)
DEF7(ulong8)
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF7(double8)
#endif // __CA_BUILTINS_DOUBLE_SUPPORT

DEF_BOTH_WAYS(long16, ulong16)
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF_BOTH_WAYS(long16, double16)
DEF_BOTH_WAYS(ulong16, double16)
#endif // __CA_BUILTINS_DOUBLE_SUPPORT

DEF(long16, long16)
DEF(ulong16, ulong16)
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF(double16, double16)
#endif // __CA_BUILTINS_DOUBLE_SUPPORT

#define DEF8(TYPE)                   \
  DEF_BOTH_WAYS(TYPE##3, u##TYPE##3) \
  DEF_BOTH_WAYS(TYPE##3, TYPE##4)    \
  DEF_BOTH_WAYS(u##TYPE##3, u##TYPE##4)

DEF8(char)
DEF8(short)
DEF8(int)
DEF8(long)

#ifdef __CA_BUILTINS_HALF_SUPPORT
DEF_BOTH_WAYS(half3, short3)
DEF_BOTH_WAYS(half3, ushort3)
DEF_BOTH_WAYS(half3, half4)
#endif

DEF_BOTH_WAYS(float3, int3)
DEF_BOTH_WAYS(float3, uint3)
DEF_BOTH_WAYS(float3, float4)

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF_BOTH_WAYS(double3, long3)
DEF_BOTH_WAYS(double3, ulong3)
DEF_BOTH_WAYS(double3, double4)
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
