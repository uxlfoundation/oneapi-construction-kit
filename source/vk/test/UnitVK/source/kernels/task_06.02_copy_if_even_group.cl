// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void copy_if_even_group(__global int *in, __global int *out) {
  size_t tid = get_global_id(0);
  int gid = get_group_id(0);
  if ((gid & 1) == 0) {
    out[tid] = in[tid];
  } else {
    out[tid] = -1;
  }
}
