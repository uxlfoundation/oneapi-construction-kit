// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Must equal workgroup size
#define SIZE 16

__kernel void local_phi(__global int* out) {
  __local int localmem_A[SIZE];
  __local int localmem_B[SIZE];

  int lid = get_local_id(0);
  if (lid == 0) {
    localmem_A[lid] = get_group_id(0);
  } else {
    localmem_B[lid] = lid;
  }

  barrier(CLK_LOCAL_MEM_FENCE);

  if (lid == 0) {
    out[get_group_id(0)] = localmem_A[0];
  }
}
