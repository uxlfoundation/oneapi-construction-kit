// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void ternary(__global int *in1, int trueVal, int falseVal,
                      __global int *out) {
  size_t tid = get_global_id(0);

  out[tid] = in1[tid] ? trueVal : falseVal;
}
