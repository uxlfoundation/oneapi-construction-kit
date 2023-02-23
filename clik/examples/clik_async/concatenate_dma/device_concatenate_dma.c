// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
