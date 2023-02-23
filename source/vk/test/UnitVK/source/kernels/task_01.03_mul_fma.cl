// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void mul_fma(__global int *in1, __global int *in2, __global int *in3,
                      __global int *out1, __global int *out2) {
  size_t tid = get_global_id(0);

  int a = in1[tid];
  int b = in2[tid];
  int c = in3[tid];
  int temp = a * b;

  // Re-use vectorized value temp.
  out1[tid] = temp;
  out2[tid] = temp + c;
}
