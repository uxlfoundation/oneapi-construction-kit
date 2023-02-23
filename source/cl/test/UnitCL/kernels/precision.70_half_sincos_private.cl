// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef TYPE
#define TYPE half
#endif

__kernel void half_sincos_private(__global TYPE* x,
                                  __global TYPE* out,
                                  __global TYPE* out_cos) {
  size_t tid = get_global_id(0);
  __private TYPE buffer[1];
  out[tid] = sincos(x[tid], &buffer[0]);
  out_cos[tid] = buffer[0];
}
