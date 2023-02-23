// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void barrier_store_mask(int mask, __global int* output) {
  int global_id = get_global_id(0);

  int gidsum = global_id;

  barrier(CLK_LOCAL_MEM_FENCE);
  int num_out = 2;
  output[global_id * num_out] = (((int)&gidsum) ^ mask) & mask;
  output[global_id * num_out + 1] = gidsum;
}
