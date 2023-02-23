// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void explicit_copy(__local int *tmpA, __local int *tmpB,
                            __global int *A, __global int *B, __global int *C) {
  size_t gid = get_global_id(0);
  size_t lid = get_local_id(0);

  tmpA[lid] = A[gid];
  tmpB[lid] = B[gid];

  barrier(CLK_LOCAL_MEM_FENCE);

  C[gid] = tmpA[lid] + tmpB[lid];
}
