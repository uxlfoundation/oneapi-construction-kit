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

__kernel void simple_mutually_exclusive_barriers(__global int* A,
                                                 __global int* B) {
  bool test = A[get_group_id(0)] == 42;
  size_t gid = get_global_id(0);
  size_t lid = get_local_id(0);

  if (test) {
    B[gid] = lid * 3;
    barrier(CLK_GLOBAL_MEM_FENCE);
  } else {
    B[gid] = lid * 3 + 1;
    barrier(CLK_GLOBAL_MEM_FENCE);
  }
}
