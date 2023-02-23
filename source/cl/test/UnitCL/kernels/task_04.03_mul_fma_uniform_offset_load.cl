// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void mul_fma_uniform_offset_load(__global int *in, __global int *out1,
                                          __global int *out2) {
  // Uniform-offset addressing.
  int gsize = get_global_size(0);
  size_t tid = get_global_id(0);

  // Layout for out: in1[0] in1[1] in2[0] in2[1] in3[0] in3[1]
  int indexIn1 = (gsize * 0) + tid;
  int indexIn2 = (gsize * 1) + tid;
  int indexIn3 = (gsize * 2) + tid;

  int a = in[indexIn1];
  int b = in[indexIn2];
  int c = in[indexIn3];
  int temp = a * b;

  out1[tid] = temp;
  out2[tid] = temp + c;
}
