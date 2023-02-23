// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half; parameters;
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef TYPE
#define TYPE half4  // cross() only defined for vec3 and vec4 types
#endif

#ifdef STORE_FUNC
#define STORE(data, offset, ptr) STORE_FUNC(data, offset, ptr)
#else
#define STORE(data, offset, ptr) vstore4(data, offset, ptr)
#endif

#ifdef LOAD_FUNC
#define LOAD(offset, ptr) LOAD_FUNC(offset, ptr)
#else
#define LOAD(offset, ptr) vload4(offset, ptr)
#endif

__kernel void half_cross(__global half* in1, __global half* in2,
                         __global half* out) {
  size_t tid = get_global_id(0);
  TYPE a = LOAD(tid, in1);
  TYPE b = LOAD(tid, in2);

  TYPE result = cross(a, b);

  STORE(result, tid, out);
}
