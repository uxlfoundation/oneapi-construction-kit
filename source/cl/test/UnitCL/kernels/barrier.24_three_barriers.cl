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

__kernel void three_barriers(__global int *A, int i) {
  // What the test does is irrelevant as long as it has three barriers
  size_t gid = get_global_id(0);

  if (i == 0) {
    barrier(CLK_LOCAL_MEM_FENCE);
    A[gid] = gid + 2;
  }
  if (i == 1) {
    barrier(CLK_LOCAL_MEM_FENCE);
    A[gid] = gid + 1;
  }
  if (i == 2) {
    barrier(CLK_LOCAL_MEM_FENCE);
    A[gid] = gid;
  }
}
