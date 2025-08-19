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
