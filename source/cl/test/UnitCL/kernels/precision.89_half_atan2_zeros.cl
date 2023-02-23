// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

__kernel void half_atan2_zeros(__global half* x,
                               __global half* y,
                               __global ushort* out_atan2,
                               __global ushort* out_atan2pi) {
  size_t id = get_global_id(0);
  half result = atan2(x[id], y[id]);
  out_atan2[id] = as_ushort(result);

  result = atan2pi(x[id], y[id]);
  out_atan2pi[id] = as_ushort(result);
}
