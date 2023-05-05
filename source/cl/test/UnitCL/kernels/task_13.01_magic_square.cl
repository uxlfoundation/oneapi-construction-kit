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

__kernel void magic_square(const __global float* src, __global float *tmp,
                           __global float *dst) {
  uint x = get_global_id(0);
  uint y = get_global_id(1);
  uint z = get_global_id(2);

  const size_t size1 = get_global_size(1);
  const size_t size2 = get_global_size(2);

  // distributed sum of 16 elements over z dimension
  size_t index_tmp = x * size2 * size1 + y * size2 + z;
  for(uint i = 0; i < 4; i++) {
    // i * 4 + z % 4 stays within range [0; 15]
    size_t index_in = x * size2 * size1 + y * size2 + i * 4 + z % 4;
    tmp[index_tmp] += src[index_in];
  }

  barrier(CLK_GLOBAL_MEM_FENCE);
  // reduce
  if(z == 0) {
    float accumulate = 0;
    for (uint k = 0; k < size2 / 4; k++) {
      size_t index_tmp_2 = x * size2 * size1 + y * size2 + k;
      accumulate += tmp[index_tmp_2];
    }
    size_t index_out = x * size1 + y;
    dst[index_out] = accumulate;
  }
  return;
}
