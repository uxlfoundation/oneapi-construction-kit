// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
