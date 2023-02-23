// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void masked_negative_argument_stride(const __global int *input,
                                              __global int *output, int stride,
                                              int size) {
  int gid = get_global_id(0);
  int index = size + (gid * stride);
  if (gid != 0) {
    output[gid] = input[index] + (gid * gid);
  } else {
    output[gid] = 13;
  }
}
