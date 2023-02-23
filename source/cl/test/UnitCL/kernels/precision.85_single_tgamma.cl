// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void single_tgamma(__global float* in, __global float *out) {
  size_t id = get_global_id(0);
  out[id] = tgamma(in[id]);
}
