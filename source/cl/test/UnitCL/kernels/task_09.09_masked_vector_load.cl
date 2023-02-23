// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void masked_vector_load(__global int *in, __global int *out,
                                 unsigned mask) {
  size_t tid = get_global_id(0);

  int8 sum = 1;
  if (!(tid & mask)) {
    sum = *(__global int8*)(in + tid);
  }

  out[tid] = sum[0];
}
