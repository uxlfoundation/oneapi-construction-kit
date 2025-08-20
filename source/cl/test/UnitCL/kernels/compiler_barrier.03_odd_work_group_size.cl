// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
