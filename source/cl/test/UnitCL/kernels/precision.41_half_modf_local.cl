// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef FLOAT_TYPE
#define FLOAT_TYPE half
#endif

__attribute__((reqd_work_group_size(4, 1, 1)))
__kernel void half_modf_local(__global FLOAT_TYPE* in, __global FLOAT_TYPE* out,
                              __global FLOAT_TYPE* out_integral) {
  size_t tid = get_global_id(0);
  size_t lid = get_local_id(0);
  __local FLOAT_TYPE buffer[4];
  __local FLOAT_TYPE *result = buffer + lid;
  out[tid] = modf(in[tid], result);
  out_integral[tid] = result[0];
}
