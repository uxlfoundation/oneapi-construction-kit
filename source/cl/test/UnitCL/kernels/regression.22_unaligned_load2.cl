// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void unaligned_load2(__global int *in, __global int *out) {
  int tid = get_global_id(0);
  int2 tmp = vload2(0, (__global int *)in + 2 * tid);
  out[2 * tid] = tmp.s0;
  out[2 * tid + 1] = tmp.s1;
}
