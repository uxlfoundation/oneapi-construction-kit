// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void unaligned_load(__global int *in, __global int *out) {
  int tid = get_global_id(0);
  int3 tmp = vload3(0, (__global int *)in + 3 * tid);
  out[3 * tid] = tmp.s0;
  out[3 * tid + 1] = tmp.s1;
  out[3 * tid + 2] = tmp.s2;
}
