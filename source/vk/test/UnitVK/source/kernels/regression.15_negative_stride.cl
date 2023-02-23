// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void negative_stride(const __global int *input, __global int *output,
                              int size) {
  int gid = get_global_id(0);
  int index = size - gid;
  output[gid] = input[index] + (gid * gid);
}
