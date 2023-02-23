// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void direct(__global int *A, __global int *B, __global int *C) {
  C[get_global_id(0)] = A[get_global_id(0)] + B[get_global_id(0)];
}
