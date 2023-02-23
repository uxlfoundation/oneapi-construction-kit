// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void explicit_copy_rotate(__local int *tmpA, __local int *tmpB,
                                   __global int *A, __global int *B,
                                   __global int *C) {
  size_t gid = get_global_id(0);
  size_t lid = get_local_id(0);

  // Each workitem loads for itself but then uses a different ID for doing the
  // calculation, i.e. only correct in the presence of the barrier.
  size_t lid2 = (lid + 1) % get_local_size(0);
  size_t gid2 = gid + (lid2 - lid);

  tmpA[lid] = A[gid];
  tmpB[lid] = B[gid];

  barrier(CLK_LOCAL_MEM_FENCE);

  C[gid2] = tmpA[lid2] + tmpB[lid2];
}
