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
// DEFINITIONS: "-DREAD_LOCAL_SIZE=4";"-DGLOBAL_ID=1"

__kernel void barrier_with_ifs(__global int* output) {
  int local_id = get_local_id(0);
  __local int cache[READ_LOCAL_SIZE];

  int output_val = 0;

  for (int d = 0; d < get_global_size(0); d++) {
    cache[local_id] = local_id;

    barrier(CLK_LOCAL_MEM_FENCE);

    if (get_global_id(0) == GLOBAL_ID) {
      // This is intentional as the bug occurs in the else.
      if (get_local_id(0) != get_local_id(0)) {
        output_val = cache[d] - 1;
      } else {
        // The contents of this if don't matter hence why its
        // the same as one of the proceeding ones.
        if (get_global_id(0) == GLOBAL_ID) {
          // Unrolled loop since a for loop here hides the bug
          // only the first cache[d] access is required
          output_val += cache[0];
          output_val += cache[1];
          output_val += cache[2];
          output_val += cache[3];
        }
      }
    }
  }

  if (get_global_id(0) == GLOBAL_ID) {
    output[0] = output_val;
  }
}
