// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void denorms(global float* A, global float* B, global float* C) {
  size_t gid = get_global_id(0);
  C[gid] = A[gid] * B[gid];
}
