// Copyright (C) Codeplay Software Limited. All Rights Reserved.
//
// DEFINITIONS: "-DARRAY_SIZE=16"

__kernel void mutually_exclusive_barriers(__global int* A, __global int* B) {
  __local int tmp[ARRAY_SIZE];

  size_t lid = get_local_id(0);

  bool test = A[get_group_id(0)] == 42;

  if (test) {
    tmp[lid] = lid * 3;
    barrier(CLK_LOCAL_MEM_FENCE);
  }

  if (!test) {
    tmp[lid] = lid * 3 + 1;
    barrier(CLK_LOCAL_MEM_FENCE);
  }

  B[get_global_id(0)] = tmp[lid];
}
