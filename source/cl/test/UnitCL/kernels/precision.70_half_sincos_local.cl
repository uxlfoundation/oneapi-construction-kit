// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef TYPE
#define TYPE half
#endif

__attribute__((reqd_work_group_size(4, 1, 1)))
__kernel void half_sincos_local(__global TYPE* x,
                                __global TYPE* out,
                                __global TYPE* out_cos) {
  size_t tid = get_global_id(0);
  size_t lid = get_local_id(0);
  __local TYPE buffer[4];
  __local TYPE *result = buffer + lid;
  out[tid] = sincos(x[tid], result);
  out_cos[tid] = result[0];
}
