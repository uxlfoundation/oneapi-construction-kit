// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void simple_mutually_exclusive_barriers(__global int* A,
                                                 __global int* B) {
  bool test = A[get_group_id(0)] == 42;
  size_t gid = get_global_id(0);
  size_t lid = get_local_id(0);

  if (test) {
    B[gid] = lid * 3;
    barrier(CLK_GLOBAL_MEM_FENCE);
  } else {
    B[gid] = lid * 3 + 1;
    barrier(CLK_GLOBAL_MEM_FENCE);
  }
}
