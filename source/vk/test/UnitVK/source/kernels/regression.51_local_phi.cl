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
