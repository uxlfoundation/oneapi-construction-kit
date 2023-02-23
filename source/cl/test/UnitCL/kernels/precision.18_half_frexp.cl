// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half parameters
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef FLOAT_TYPE
#define FLOAT_TYPE half
#endif

#ifndef INT_TYPE
#define INT_TYPE int
#endif

__kernel void half_frexp(__global FLOAT_TYPE* a, __global FLOAT_TYPE* out,
                         __global INT_TYPE* out_int) {
  size_t tid = get_global_id(0);
  out[tid] = frexp(a[tid], &out_int[tid]);
}
