// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void copy_if_even_item_phi(__global int *in, __global int *out) {
  size_t tid = get_global_id(0);
  size_t lid = get_local_id(0);
  int result;
  if ((lid & 1) == 0) {
    result = in[tid];
  } else {
    result = -1;
  }
  out[tid] = result;
}
