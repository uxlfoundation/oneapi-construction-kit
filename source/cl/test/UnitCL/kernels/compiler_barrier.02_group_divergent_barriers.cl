// Copyright (C) Codeplay Software Limited. All Rights Reserved.
//
// DEFINITIONS: "-DARRAY_SIZE=16"

__kernel void group_divergent_barriers(__global int* A, __global int* B) {
  __local int tmp[ARRAY_SIZE];

  size_t lid = get_local_id(0);

  if (A[get_group_id(0)] == 42) {
    tmp[lid] = lid;
    barrier(CLK_LOCAL_MEM_FENCE);
  } else {
    tmp[lid] = lid + 1;
    barrier(CLK_LOCAL_MEM_FENCE);
  }

  B[get_global_id(0)] = tmp[lid];
}
