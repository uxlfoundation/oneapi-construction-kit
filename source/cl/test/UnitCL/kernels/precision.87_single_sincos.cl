// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void single_sincos(__global float* in,
                            __global float* out_sin,
                            __global float* out_cos) {
  size_t id = get_global_id(0);
  out_sin[id] = sincos(in[id], &out_cos[id]);
}
