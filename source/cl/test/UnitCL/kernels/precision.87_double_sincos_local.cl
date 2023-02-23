// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: double

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__attribute__((reqd_work_group_size(16, 1, 1)))
__kernel void double_sincos_local(__global double* in,
                                  __global double* out_sin,
                                  __global double* out_cos) {
  size_t id = get_global_id(0);
  size_t lid = get_local_id(0);

  __local double scratch[16];
  __local double *result = scratch + lid;
  out_sin[id] = sincos(in[id], result);
  out_cos[id] = result[0];
}
