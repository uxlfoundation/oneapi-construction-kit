// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void masked_argument_stride(const __global int *input,
                                     __global int *output, int stride) {
  size_t gid = get_global_id(0);
  int index = get_global_id(0) * stride;
  if (gid != 0) {
    output[index] = input[index];
    output[index + 1] = 1;
    output[index + 2] = 1;
  } else {
    output[index] = 13;
    output[index + 1] = 13;
    output[index + 2] = 13;
  }
}
