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

// This should fail to DMA as the lookup is conditional
__kernel void auto_dma_loop_convolution_cond_round_inner_loop(
    __global uint *src, __global uint *dst) {
  uint dstYStride = get_global_size(0);
  uint dstIndex = get_global_id(1) * dstYStride + get_global_id(0);
  uint srcYStride = dstYStride + 16;
  uint srcIndex = get_global_id(1) * srcYStride + get_global_id(0) + 8;
  srcIndex += srcYStride;
  uint count = 9;
  int y = 0;
  for (int y = 0; y < 3; y++) {
    if (y == 1) {
      for (int x = 0; x < 3; x++) {
        count = count + src[srcYStride * y + srcIndex + x - 1];
      }
    }
  }
  // Write data out
  dst[dstIndex] = count / 9;
}
