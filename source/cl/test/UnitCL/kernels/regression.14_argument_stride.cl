// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void argument_stride(const __global int *input, __global int *output,
                              int stride) {
  int index = get_global_id(0) * stride;
  output[index] = input[index];
  output[index + 1] = 1;
  output[index + 2] = 1;
}
