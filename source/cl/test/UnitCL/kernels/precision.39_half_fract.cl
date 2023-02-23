// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef FLOAT_TYPE
#define FLOAT_TYPE half
#endif

__kernel void half_fract(__global FLOAT_TYPE* a, __global FLOAT_TYPE* out,
                         __global FLOAT_TYPE* out_floor) {
  size_t tid = get_global_id(0);
  out[tid] = fract(a[tid], &out_floor[tid]);
}
