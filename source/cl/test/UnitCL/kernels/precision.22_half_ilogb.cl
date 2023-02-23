// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef TYPE
#define TYPE half
#endif

#ifndef INT_TYPE
#define INT_TYPE int
#endif

__kernel void half_ilogb(__global TYPE* a, __global INT_TYPE* out) {
  size_t tid = get_global_id(0);
  out[tid] = ilogb(a[tid]);
}
