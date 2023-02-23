// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "kernel_if.h"

#define TS 4  // Tile size

__kernel void matrix_multiply(__global float *a, __global float *b,
                              __global float *c, uint m, exec_state_t *item) {
  uint col = get_global_id(0, item);
  uint row = get_global_id(1, item);
  uint lx = get_local_id(0, item);
  uint ly = get_local_id(1, item);

  __local_variable float tile_a[TS * TS];
  __local_variable float tile_b[TS * TS];
  float sum = 0.0f;
  for (int i = 0; i < m; i += TS) {
    // Load one tile from A and B into local memory.
    tile_a[(ly * TS) + lx] = a[(row * m) + i + lx];
    tile_b[(ly * TS) + lx] = b[((i + ly) * m) + col];
    item->barrier(item);

    // Compute the partial sum for the loaded tile.
    for (int j = 0; j < TS; j++) {
      sum += tile_a[(ly * TS) + j] * tile_b[(j * TS) + lx];
    }

    // Wait for all items to be finished before computing the next tile.
    item->barrier(item);
  }
  c[(row * m) + col] = sum;
}

typedef struct {
  __global float *a;
  __global float *b;
  __global float *c;
  uint m;
} matrix_multiply_args;

// Execute the kernel once for each work-group contained in a work-slice.
// This function is called on each hardware thread of the device and there are
// as many slices as hardware threads.
int kernel_main(const matrix_multiply_args *args, exec_state_t *ctx) {
  ctx->local_id[0] = ctx->thread_id % TS;
  ctx->local_id[1] = ctx->thread_id / TS;
  for (uint i = 0; i < ctx->num_groups[0]; i++) {
    ctx->group_id[0] = i;
    for (uint j = 0; j < ctx->num_groups[1]; j++) {
      ctx->group_id[1] = j;
      matrix_multiply(args->a, args->b, args->c, args->m, ctx);
    }
  }
  return 0;
}
