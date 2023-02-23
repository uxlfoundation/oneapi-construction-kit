// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: double

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void double_sincos(__global double* in,
                            __global double* out_sin,
                            __global double* out_cos) {
  size_t id = get_global_id(0);
  out_sin[id] = sincos(in[id], &out_cos[id]);
}
