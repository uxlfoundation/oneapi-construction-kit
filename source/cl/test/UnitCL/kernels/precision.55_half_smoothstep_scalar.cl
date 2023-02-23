// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef TYPE
#define TYPE half
#endif

__kernel void half_smoothstep_scalar(__global half* a, __global half* b,
                                     __global TYPE* c, __global TYPE* out) {
  size_t tid = get_global_id(0);
  out[tid] = smoothstep(a[tid], b[tid], c[tid]);
}
