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
// DEFINITIONS: "-DREAD_LOCAL_SIZE=16";"-DOUTER_LOOP_SIZE=1";"-DGLOBAL_ID=0"

__kernel void barrier_in_loop(__global int* output) {
  int global_id = get_global_id(0);
  int local_id = get_local_id(0);
  __local int cache[READ_LOCAL_SIZE];

  int output_val = 0;

  for (int d = 0; d < OUTER_LOOP_SIZE; d++) {
    cache[local_id] = local_id;
    barrier(CLK_LOCAL_MEM_FENCE);

    for (int i = 0; i < READ_LOCAL_SIZE; i++) {
      output_val += cache[i];
    }
  }

  if (global_id == GLOBAL_ID) {
    output[0] = output_val;
  }
}
