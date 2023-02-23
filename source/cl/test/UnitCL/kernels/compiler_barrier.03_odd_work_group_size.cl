// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: parameters

__kernel void odd_work_group_size(__global int* A, __global int* B) {
  __local int tmpIn[ARRAY_SIZE];
  __local int tmpOut[ARRAY_SIZE];

  size_t lid = get_local_id(0);
  size_t gid = get_global_id(0);

  tmpIn[lid] = A[gid];
  barrier(CLK_LOCAL_MEM_FENCE);

  if (lid == 0) {
    if (get_local_size(0) > 1) {
      tmpOut[lid] = tmpIn[lid] + tmpIn[lid + 1];
    } else {
      tmpOut[lid] = tmpIn[lid];
    }
  } else {
    tmpOut[lid] = tmpIn[lid - 1] + tmpIn[lid];
  }

  B[gid] = tmpOut[lid];
}
