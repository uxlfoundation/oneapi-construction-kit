// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void explicit_copy_rotate_compare(__local int *tmpA, __local int *tmpB,
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

  int result = tmpA[lid2] + tmpB[lid2];

#define CASE(X)                           \
  case (X):                               \
    C[gid] = (result == ((X)*2)) ? 1 : 2; \
    break

  switch (lid2) {
    CASE(0);
    CASE(1);
    CASE(2);
    CASE(3);
    CASE(4);
    CASE(5);
    CASE(6);
    CASE(7);
    CASE(8);
    CASE(9);
    CASE(10);
    CASE(11);
    CASE(12);
    CASE(13);
    CASE(14);
    CASE(15);
    CASE(16);
    CASE(17);
    CASE(18);
    CASE(19);
    CASE(20);
    CASE(21);
    CASE(22);
    CASE(23);
    CASE(24);
    CASE(25);
    CASE(26);
    CASE(27);
    CASE(28);
    CASE(29);
    CASE(30);
    CASE(31);
    default:
      C[gid] = 3;
      break;
  }
}
