// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void single_lgamma(__global float* in, __global float *out) {
  size_t id = get_global_id(0);
  float2 in2 = (float2)(in[id * 2 + 0], in[id * 2 + 1]);
  float2 ret = lgamma(in2);
  out[id * 2 + 0] = ret[0];
  out[id * 2 + 1] = ret[1];
}
