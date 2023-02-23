// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void gather(__global int *in, __global int *out) {
  size_t tid = get_global_id(0);

  out[tid] = in[tid * 2];
}
