// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef FLOAT_TYPE
#define FLOAT_TYPE half
#endif

__attribute__((reqd_work_group_size(4, 1, 1)))
__kernel void half_fract_local(__global FLOAT_TYPE* a, __global FLOAT_TYPE* out,
                               __global FLOAT_TYPE* out_floor) {
  size_t tid = get_global_id(0);
  size_t lid = get_local_id(0);
  __local FLOAT_TYPE buffer[4];
  __local FLOAT_TYPE *result = buffer + lid;
  out[tid] = fract(a[tid], result);
  out_floor[tid] = result[0];
}
