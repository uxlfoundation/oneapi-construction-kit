// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void barrier_local_arrays(__global float* restrict input,
                                   __global float* restrict result) {
  float accum[4];
  __local float tmp[256];
  for (int i = 0; i < 4; ++i) {
    accum[i] = 0.0f;
  }
  for (int z = 0; z < 64; ++z) {
    // the inner loop is necessary to manifest the bug,
    // even though its value is never used.
    for (int y = 0; y < 2; ++y) {
      barrier(CLK_LOCAL_MEM_FENCE);

      int r = get_local_id(1) * 16 + get_local_id(0);
      tmp[r] = input[(r * 64 + z) & 0x3ff];

      barrier(CLK_LOCAL_MEM_FENCE);
      for (int x = 0; x < 16; ++x) {
        int b = get_local_id(1) * 16 + x;
        accum[x & 0x3] += input[b] * tmp[b];
      }
    }
  }
  for (int i = 0; i < 4; ++i) {
    int b = get_local_id(1) * 4 + i;
    result[b] = accum[i];
  }
}
