// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void add(__global int *in1, __global int *in2, __global int *out) {
  size_t tid = get_global_id(0);
  // Vectorize load and add chain.
  out[tid] = in1[tid] + in2[tid];
}
