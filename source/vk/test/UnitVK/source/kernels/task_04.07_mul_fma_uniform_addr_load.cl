// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void mul_fma_uniform_addr_load(__global int *in, __global int *out1,
                                        __global int *out2) {
  // Uniform-offset addressing.
  size_t lid = get_local_id(0);
  size_t lsize = get_local_size(0);
  int base = get_global_offset(0) + (get_group_id(0) * lsize);
  size_t tid = base + lid;

  // Layout for in: in1[0] in2[0] in3[0] in1[1] in2[1] in3[1]
  int indexIn1 = (base * 3) + (lsize * 0) + lid;
  int indexIn2 = (base * 3) + (lsize * 1) + lid;
  int indexIn3 = (base * 3) + (lsize * 2) + lid;

  int a = in[indexIn1];
  int b = in[indexIn2];
  int c = in[indexIn3];
  int temp = a * b;

  out1[tid] = temp;
  out2[tid] = temp + c;
}
