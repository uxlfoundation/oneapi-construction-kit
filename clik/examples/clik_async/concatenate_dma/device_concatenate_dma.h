// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _CLIK_EXAMPLES_CLIK_ASYNC_CONCATENATE_DMA_H
#define _CLIK_EXAMPLES_CLIK_ASYNC_CONCATENATE_DMA_H

#include "kernel_if.h"

__kernel void concatenate_dma(__global uint *src1, __global uint *src2,
                              __global uint *dst, uint block_size,
                              exec_state_t *ctx);

typedef struct {
  __global uint *src1;
  __global uint *src2;
  __global uint *dst;
  uint block_size;
} concatenate_dma_args;

#endif  // _CLIK_EXAMPLES_CLIK_ASYNC_CONCATENATE_DMA_H
