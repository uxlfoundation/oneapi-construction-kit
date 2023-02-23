// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void single_group_3d(__global int *A, uint groupX, uint groupY,
                              uint groupZ) {
  size_t id = get_global_id(0) + get_global_id(1) * get_global_size(0) +
              get_global_id(2) * get_global_size(0) * get_global_size(1);
  A[id] = 0;

  barrier(CLK_GLOBAL_MEM_FENCE);

  if ((get_group_id(0) == groupX) && (get_group_id(1) == groupY) &&
      (get_group_id(2) == groupZ)) {
    A[id] = 1;
  }
}
