// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void varying_alloca(__global int4 *input, __global int4 *results) {
  int4 cache[64];
  int tid = get_global_id(0);

  // This for loop and its instructions are uniform
  for (int i = 0; i < 64; i++) {
    cache[i] = input[i];
  }

  // These instructions are varying
  results[tid] = vload4(tid % 64, (int *)cache);
}
