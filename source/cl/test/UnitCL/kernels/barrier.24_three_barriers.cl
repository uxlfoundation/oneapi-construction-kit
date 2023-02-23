// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void three_barriers(__global int *A, int i) {
  // What the test does is irrelevant as long as it has three barriers
  size_t gid = get_global_id(0);

  if (i == 0) {
    barrier(CLK_LOCAL_MEM_FENCE);
    A[gid] = gid + 2;
  }
  if (i == 1) {
    barrier(CLK_LOCAL_MEM_FENCE);
    A[gid] = gid + 1;
  }
  if (i == 2) {
    barrier(CLK_LOCAL_MEM_FENCE);
    A[gid] = gid;
  }
}
