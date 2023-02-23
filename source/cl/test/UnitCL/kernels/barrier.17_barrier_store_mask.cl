// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void barrier_store_mask(int mask, __global int* output) {
  int global_id = get_global_id(0);
  barrier(CLK_LOCAL_MEM_FENCE);
  output[global_id] = (((int)&global_id) & mask);
}
