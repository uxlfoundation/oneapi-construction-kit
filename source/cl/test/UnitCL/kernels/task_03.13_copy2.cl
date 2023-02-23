// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void copy2(__global int2 *in, __global int2 *out) {
  size_t tid = get_global_id(0);

  out[tid] = in[tid];
}
