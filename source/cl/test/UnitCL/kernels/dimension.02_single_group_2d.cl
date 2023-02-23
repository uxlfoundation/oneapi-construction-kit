// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void single_group_2d(__global int *A, uint groupX, uint groupY) {
  size_t id = get_global_id(0) + get_global_id(1) * get_global_size(0);
  A[id] = 0;

  barrier(CLK_GLOBAL_MEM_FENCE);

  if ((get_group_id(0) == groupX) && (get_group_id(1) == groupY)) {
    A[id] = 1;
  }
}
