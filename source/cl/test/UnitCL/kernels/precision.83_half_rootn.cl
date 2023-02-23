// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef FLOAT_TYPE
#define FLOAT_TYPE half
#endif

#ifndef INT_TYPE
#define INT_TYPE int
#endif

__kernel void half_rootn(__global FLOAT_TYPE* a,
                         __global INT_TYPE* b,
                         __global FLOAT_TYPE* out) {
  size_t tid = get_global_id(0);
  out[tid] = rootn(a[tid], b[tid]);
}
