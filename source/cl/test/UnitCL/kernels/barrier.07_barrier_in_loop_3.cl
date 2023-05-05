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
//
// DEFINITIONS: "-DREAD_LOCAL_SIZE=4";"-DGLOBAL_ID=0"

__kernel void barrier_in_loop_3(__global int* output) {
  int global_id = get_global_id(0);
  int local_id = get_local_id(0);
  __local int cache[READ_LOCAL_SIZE];

  int my_value = 0;

  for (unsigned int i = 0; i < get_local_size(0);) {
    // The access to cache at this stage is required to reveal the bug
    // however, the if condition is intentionally not met.
    if (i > get_local_size(0) / 2) {
      my_value = cache[local_id];
    }

    unsigned int local_size = get_local_size(0);
    for (unsigned int j = 0; j < local_size; j++) {
      cache[local_id] = local_id;

      // This is the barrier that can cause issues in certain cases.
      barrier(CLK_LOCAL_MEM_FENCE);

      // Unrolled loop so we don't hide the bug
      my_value += cache[0];
      my_value += cache[1];
      my_value += cache[2];
      my_value += cache[3];
    }

    // The access to cache at this stage is required to reveal the bug
    // however, the if condition is intentionally not met.
    if (get_local_id(0) > get_local_size(0)) {
      my_value = cache[get_local_id(0)];
    }
    i += local_size;
  }

  if (get_global_id(0) == GLOBAL_ID) {
    output[0] = my_value;
  }
}
