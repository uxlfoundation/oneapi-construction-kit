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

#include "device_concatenate_dma.h"

// Carry out the computation for one work-item.
__kernel void concatenate_dma(__global uint *src1, __global uint *src2,
                              __global uint *dst, uint block_size,
                              exec_state_t *ctx) {
  uint tid = get_global_id(0, ctx);
  uint offset = tid * block_size;
  uint size = get_global_size(0, ctx) * block_size;

  // Enqueue a DMA operation from the first input buffer to the first half of
  // the output buffer.
  uintptr_t xfer1 =
      start_dma(&dst[offset], &src1[offset], block_size * sizeof(uint), ctx);

  // Enqueue a DMA operation from the second input buffer to the second half of
  // the output buffer, without waiting for the first DMA operation to finish.
  uintptr_t xfer2 = start_dma(&dst[size + offset], &src2[offset],
                              block_size * sizeof(uint), ctx);

  // Wait for both DMA operations to finish. Waiting for a DMA operation only
  // returns when that operation as well as all other operations enqueued before
  // it are completed.
  wait_dma(xfer2, ctx);
}
