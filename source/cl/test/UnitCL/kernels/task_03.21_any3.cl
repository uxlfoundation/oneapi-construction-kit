// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void any3(__global int3 *in, __global int *out) {
  size_t tid = get_global_id(0);
  int3 src = vload3(tid, (__global int *)in);
  out[tid] = any(src);
}
