// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

__kernel void half_hypot_edgecases(__global half* x,
                                   __global half* y,
                                   __global half* out) {
  size_t id = get_global_id(0);
  out[id] = hypot(x[id], y[id]);
}
