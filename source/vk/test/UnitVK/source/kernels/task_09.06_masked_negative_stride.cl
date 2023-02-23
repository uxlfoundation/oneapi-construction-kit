// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void masked_negative_stride(const __global int *input,
                                     __global int *output, int size) {
  size_t gid = get_global_id(0);
  int index = size - gid;
  if (gid != 0) {
    output[gid] = input[index] + (gid * gid);
  } else {
    output[gid] = 13;
  }
}
