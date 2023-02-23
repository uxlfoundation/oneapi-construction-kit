// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void copy_if_even_item(__global int *in, __global int *out) {
  size_t tid = get_global_id(0);
  size_t lid = get_local_id(0);
  if ((lid & 1) == 0) {
    out[tid] = in[tid];
  } else {
    out[tid] = -1;
  }
}
