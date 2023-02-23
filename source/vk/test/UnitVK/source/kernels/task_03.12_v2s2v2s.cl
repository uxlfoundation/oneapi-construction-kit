// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void v2s2v2s(__global int4 *in, __global int *out) {
  size_t tid = get_global_id(0);

  int4 v = in[tid];
  uint sum = v.x + v.y + v.z + v.w;
  uint4 v2 = (uint4)sum + (uint4)(1u, 2u, 3u, 4u);
  uint sum2 = v2.x * v2.y * v2.z * v2.w;

  out[tid] = sum2;
}
