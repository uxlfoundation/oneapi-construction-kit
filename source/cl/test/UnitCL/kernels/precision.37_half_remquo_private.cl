// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef FLOAT_TYPE
#define FLOAT_TYPE half
#endif

#ifndef INT_TYPE
#define INT_TYPE int
#endif

__kernel void half_remquo_private(__global FLOAT_TYPE* x,
                                  __global FLOAT_TYPE* y,
                                  __global FLOAT_TYPE* out,
                                  __global INT_TYPE* out_int) {
  size_t tid = get_global_id(0);
  __private INT_TYPE buffer[1];
  out[tid] = remquo(x[tid], y[tid], &buffer[0]);
  out_int[tid] = buffer[0];
}
