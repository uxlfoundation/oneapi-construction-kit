// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void copy_constant_offset(__global int *in, __global int *out) {
  size_t tid = get_global_id(0);

  out[4 + tid] = in[tid];
}
