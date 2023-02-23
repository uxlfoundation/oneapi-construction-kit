// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _CLIK_EXAMPLES_CLIK_ASYNC_MATRIX_MULTIPLY_TILED_H
#define _CLIK_EXAMPLES_CLIK_ASYNC_MATRIX_MULTIPLY_TILED_H

#include "kernel_if.h"

#define TS 4  // Tile size

__kernel void matrix_multiply(__global float *a, __global float *b,
                              __global float *c, uint m, exec_state_t *ctx);

typedef struct {
  __global float *a;
  __global float *b;
  __global float *c;
  uint m;
} matrix_multiply_args;

#endif  // _CLIK_EXAMPLES_CLIK_ASYNC_MATRIX_MULTIPLY_TILED_H
