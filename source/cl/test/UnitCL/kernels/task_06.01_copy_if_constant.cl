// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void copy_if_constant(__global int *in, __global int *out,
                               const int answer) {
  size_t tid = get_global_id(0);
  if (answer == 42) {
    out[tid] = in[tid];
  }
}
