// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half; parameters;
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef TYPE
#define TYPE half
#endif

#ifdef LOAD_FUNC
#define LOAD(offset, ptr) LOAD_FUNC(offset, ptr)
#else
#define LOAD(offset, ptr) ptr[tid]
#endif

__kernel void half_length(__global half* in, __global half* out) {
  size_t tid = get_global_id(0);
  TYPE a = LOAD(tid, in);

  out[tid] = length(a);
}
