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

// DEFINITIONS: "-DREAD_LOCAL_SIZE=16";"-DGLOBAL_ID=1";"-DREAD_LOCAL_ID=1"

void __attribute__((overloadable))
barrier(cl_mem_fence_flags flags);

void Barrier_Function() { barrier(CLK_LOCAL_MEM_FENCE); }

__kernel void barrier_local_mem(__global const int* input,
                                __global int* output) {
  __local int cache[READ_LOCAL_SIZE];

  int global_id = get_global_id(0);
  int local_id = get_local_id(0);

  Barrier_Function();  // Double barrier

  cache[local_id] = input[global_id];

  Barrier_Function();  // Double barrier

  if (global_id == GLOBAL_ID) {
    output[0] = cache[READ_LOCAL_ID];
  }
}
