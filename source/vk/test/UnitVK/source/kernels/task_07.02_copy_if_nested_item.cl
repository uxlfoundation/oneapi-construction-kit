// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void copy_if_nested_item(__global int *in, __global int *out1,
                                  __global int *out2) {
  size_t tid = get_global_id(0);
  size_t lid = get_local_id(0);
  if ((lid & 1) == 0) {
    int value = in[tid];
    if ((lid & 2) == 0) {
      out1[tid] = -value;
    }
    out2[tid] = value;
  }
}
