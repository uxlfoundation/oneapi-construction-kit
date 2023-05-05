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

// This one should succeed the DMA as the conditional is common across the group
__kernel void auto_dma_loop_convolution_cond_not_global_id(__global uint *src,
                                                           __global uint *dst,
                                                           uint extraParam) {
  size_t dstYStride = get_global_size(0);
  size_t dstIndex = get_global_id(1) * dstYStride + get_global_id(0);
  size_t srcYStride = dstYStride + 16;
  size_t srcIndex = get_global_id(1) * srcYStride + get_global_id(0) + 8;
  srcIndex += srcYStride;
  uint count = 9;
  if (extraParam > 1) {
    count += 10;
  }
  for (size_t y = 0; y < 3; y++) {
    for (size_t x = 0; x < 3; x++) {
      count = count + src[srcYStride * y + srcIndex + x - 1];
    }
  }
  // Write data out
  dst[dstIndex] = count / 9;
}
