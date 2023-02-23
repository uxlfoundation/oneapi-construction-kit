// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef UNITVK_KTS_CLSPV_COMMON_H_INCLUDED
#define UNITVK_KTS_CLSPV_COMMON_H_INCLUDED

#include "kts_vk.h"
#include "kts/reference_functions.h"

typedef uint16_t cl_ushort;

static_assert(sizeof(glsl::intTy) == 4, "cl_int is a 32-bit signed integer");
typedef glsl::intTy cl_int;

static_assert(sizeof(glsl::ivec2Ty) == 8, "cl_int2 is a 2*32-bit wide");
typedef glsl::ivec2Ty cl_int2;

static_assert(sizeof(glsl::uintTy) == 4, "cl_uint is a 32-bit unsigned integer");
typedef glsl::uintTy cl_uint;

static_assert(sizeof(glsl::uvec4Ty) == 16, "cl_uint4 is 4*32-bit wide");
typedef glsl::uvec4Ty cl_uint4;

typedef glsl::floatTy cl_float;

static_assert(sizeof(glsl::vec4Ty) == 16, "cl_float4 is 4*32-bit wide");
typedef glsl::vec4Ty cl_float4;

static_assert(sizeof(glsl::ivec4Ty) == 16, "cl_int4 is 4*32-bit wide");
typedef glsl::ivec4Ty cl_int4;

#endif  // UNITVK_KTS_CLSPV_COMMON_H_INCLUDED
