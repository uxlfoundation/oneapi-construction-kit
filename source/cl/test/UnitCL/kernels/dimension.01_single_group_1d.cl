// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void single_group_1d(__global int *A, uint groupX) {
  size_t id = get_global_id(0);
  A[id] = 0;

  barrier(CLK_GLOBAL_MEM_FENCE);

  if (get_group_id(0) == groupX) {
    A[id] = 1;
  }
}
