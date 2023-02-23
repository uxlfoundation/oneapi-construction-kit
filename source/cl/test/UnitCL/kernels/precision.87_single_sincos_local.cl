// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__attribute__((reqd_work_group_size(16, 1, 1)))
__kernel void single_sincos_local(__global float* in,
                                  __global float* out_sin,
                                  __global float* out_cos) {
  size_t id = get_global_id(0);
  size_t lid = get_local_id(0);

  __local float scratch[16];
  __local float *result = scratch + lid;
  out_sin[id] = sincos(in[id], result);
  out_cos[id] = result[0];
}
