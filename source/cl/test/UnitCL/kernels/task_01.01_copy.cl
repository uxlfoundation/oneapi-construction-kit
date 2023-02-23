// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void copy(__global int *in, __global int *out) {
  size_t tid = get_global_id(0);
  // Vectorize load.
  out[tid] = in[tid];
}
