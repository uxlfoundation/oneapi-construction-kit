// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void mul_fma_uniform_addr_store(__global int *in1, __global int *in2,
                                         __global int *in3, __global int *out) {
  // Uniform-offset addressing.
  size_t lid = get_local_id(0);
  size_t lsize = get_local_size(0);
  int base = get_global_offset(0) + (get_group_id(0) * lsize);
  size_t tid = base + lid;

  // Layout for out: out1[0] out2[0] out1[1] out2[1]
  int indexOut1 = (base * 2) + (lsize * 0) + lid;
  int indexOut2 = (base * 2) + (lsize * 1) + lid;

  int a = in1[tid];
  int b = in2[tid];
  int c = in3[tid];
  int temp = a * b;

  out[indexOut1] = temp;
  out[indexOut2] = temp + c;
}
