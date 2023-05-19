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

__kernel void barrier_local_arrays(__global float* restrict input,
                                   __global float* restrict result) {
  float accum[4];
  __local float tmp[256];
  for (int i = 0; i < 4; ++i) {
    accum[i] = 0.0f;
  }
  for (int z = 0; z < 64; ++z) {
    // the inner loop is necessary to manifest the bug,
    // even though its value is never used.
    for (int y = 0; y < 2; ++y) {
      barrier(CLK_LOCAL_MEM_FENCE);

      int r = get_local_id(1) * 16 + get_local_id(0);
      tmp[r] = input[(r * 64 + z) & 0x3ff];

      barrier(CLK_LOCAL_MEM_FENCE);
      for (int x = 0; x < 16; ++x) {
        int b = get_local_id(1) * 16 + x;
        accum[x & 0x3] += input[b] * tmp[b];
      }
    }
  }
  for (int i = 0; i < 4; ++i) {
    int b = get_local_id(1) * 4 + i;
    result[b] = accum[i];
  }
}
