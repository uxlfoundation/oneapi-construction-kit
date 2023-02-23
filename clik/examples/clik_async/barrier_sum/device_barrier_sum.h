// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _CLIK_EXAMPLES_CLIK_ASYNC_BARRIER_SUM_H
#define _CLIK_EXAMPLES_CLIK_ASYNC_BARRIER_SUM_H

#include "kernel_if.h"

__kernel void barrier_sum(__global uint *src, __global uint *dst,
                          __local uint *src_tile, exec_state_t *ctx);

typedef struct {
  __global uint *src;
  __global uint *dst;
  __local uint *src_tile;
} barrier_sum_args;

#endif  // _CLIK_EXAMPLES_CLIK_ASYNC_BARRIER_SUM_H
