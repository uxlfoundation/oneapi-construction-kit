// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef TYPE
#define TYPE half
#endif

__kernel void half_atan2(__global TYPE* x, __global TYPE* y,
                         __global TYPE* out) {
  size_t tid = get_global_id(0);
  out[tid] = atan2(x[tid], y[tid]);
}
