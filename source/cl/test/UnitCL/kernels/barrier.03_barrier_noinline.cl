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

void __attribute__((overloadable, noinline)) barrier(cl_mem_fence_flags flags);

void Barrier_Function() { barrier(CLK_LOCAL_MEM_FENCE); }

__kernel void barrier_noinline(__global int* ptr, __global int* ptr2) {
  __local int tile[2];

  int idx = get_global_id(0);
  int pos = idx & 1;
  int opp = pos ^ 1;

  Barrier_Function();  // Double barrier

  tile[pos] = ptr[idx];

  Barrier_Function();  // Double barrier

  ptr[idx] = tile[opp];
  ptr2[idx] = tile[opp];
}
