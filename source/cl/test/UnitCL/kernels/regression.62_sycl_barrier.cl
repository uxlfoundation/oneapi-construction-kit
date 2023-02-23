// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void sycl_barrier(__global int* A, __local int* localPtrA,
                           __global int* B, __local int* localPtrB,
                           __global int* C, __local int* localPtrC,
                           __global int* D, __local int* localPtrD) {
  int globalID =
      (get_global_id(2) * (get_global_size(0) * get_global_size(1))) +
      (get_global_id(1) * get_global_size(0)) + (get_global_id(0));
  int localID = (get_local_id(2) * (get_local_size(0) * get_local_size(1))) +
                (get_local_id(1) * get_local_size(0)) + (get_local_id(0));

  localPtrA[localID] = A[globalID];
  localPtrB[localID] = B[globalID];
  localPtrC[localID] = C[globalID];
  localPtrD[localID] = D[globalID];

  barrier(CLK_LOCAL_MEM_FENCE);

  A[globalID] = localPtrA[localID ^ 1];
  B[globalID] = localPtrB[localID ^ 1];
  C[globalID] = localPtrC[localID ^ 1];
  D[globalID] = localPtrD[localID ^ 1];
}
