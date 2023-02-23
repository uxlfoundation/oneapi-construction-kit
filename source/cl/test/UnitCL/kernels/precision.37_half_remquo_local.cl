// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef FLOAT_TYPE
#define FLOAT_TYPE half
#endif

#ifndef INT_TYPE
#define INT_TYPE int
#endif

__attribute__((reqd_work_group_size(4, 1, 1)))
__kernel void half_remquo_local(__global FLOAT_TYPE* x,
                                __global FLOAT_TYPE* y,
                                __global FLOAT_TYPE* out,
                                __global INT_TYPE* out_int) {
  size_t tid = get_global_id(0);
  size_t lid = get_local_id(0);
  __local INT_TYPE buffer[4];
  __local INT_TYPE *result = buffer + lid;
  out[tid] = remquo(x[tid], y[tid], result);
  out_int[tid] = result[0];
}
