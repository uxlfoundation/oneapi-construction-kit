// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef TYPE
#define TYPE half
#endif

#ifdef LOAD_FUNC
#define LOAD(offset, ptr) LOAD_FUNC(offset, ptr)
#else
#define LOAD(offset, ptr) ptr[tid]
#endif

#ifdef STORE_FUNC
#define STORE(data, offset, ptr) STORE_FUNC(data, offset, ptr)
#else
#define STORE(data, offset, ptr) ptr[tid] = data
#endif

__kernel void half_div(__global half* a, __global half* b, __global half* out) {
  size_t tid = get_global_id(0);
  const TYPE x = LOAD(tid, a);
  const TYPE y = LOAD(tid, b);
  const TYPE result = x / y;
  STORE(result, tid, out);
}
