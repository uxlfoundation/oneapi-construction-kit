// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void mul_fma_uniform_offset_store(__global int *in1, __global int *in2,
                                           __global int *in3,
                                           __global int *out) {
  // Uniform-offset addressing.
  int gsize = get_global_size(0);
  size_t tid = get_global_id(0);

  // Layout for out: out1[0] out1[1] out2[0] out2[1]
  int indexOut1 = (gsize * 0) + tid;
  int indexOut2 = (gsize * 1) + tid;

  int a = in1[tid];
  int b = in2[tid];
  int c = in3[tid];
  int temp = a * b;

  out[indexOut1] = temp;
  out[indexOut2] = temp + c;
}
