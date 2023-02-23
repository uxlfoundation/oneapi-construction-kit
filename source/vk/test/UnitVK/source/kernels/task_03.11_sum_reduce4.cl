// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void sum_reduce4(__global int4 *in, __global int *out) {
  size_t tid = get_global_id(0);

  int4 v = in[tid];
  int sum = v.x + v.y + v.z + v.w;

  out[tid] = sum;
}
