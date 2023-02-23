// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "kernel_if.h"

// In this example, each work-item in the kernel computes the sum of a few
// values. These values are the same for all items in the work-group. In order
// to reduce the number of memory operations, a small local array is used to
// share these values between the items in the group. Each item only needs to
// load one item from the source buffer, however a barrier is needed to ensure
// all values have been copied before computing the sum.

__kernel void barrier_sum(__global uint *src, __global uint *dst,
                          __local uint *src_tile, exec_state_t *item) {
  uint tid = get_global_id(0, item);

  // Copy values from the source buffer to a local tile.
  uint lid = get_local_id(0, item);
  src_tile[lid] = src[tid];

  // Wait for all items in the group to have finished copying values.
  item->barrier(item);

  // Sum values from the tile and write the result to the output buffer.
  uint sum = 0;
  for (uint i = 0; i < get_local_size(0, item); i++) {
    sum += src_tile[i];
  }
  dst[tid] = sum;
}

typedef struct {
  __global uint *src;
  __global uint *dst;
  __local uint *src_tile;
} barrier_sum_args;

// Execute the kernel once for each work-group contained in a work-slice.
// This function is called on each hardware thread of the device and there are
// as many slices as hardware threads.
int kernel_main(const barrier_sum_args *args, exec_state_t *ctx) {
  ctx->local_id[0] = ctx->thread_id;
  for (uint i = 0; i < ctx->num_groups[0]; i++) {
    ctx->group_id[0] = i;
    barrier_sum(args->src, args->dst, args->src_tile, ctx);

    // When local memory is used, a barrier is needed between work-groups to
    // ensure that all work-items in the group have finished executing before
    // starting the next group. Otherwise, a work-item from the 'next' group
    // might overwrite the data used by a work-item from the 'previous' group.
    ctx->barrier(ctx);
  }
  return 0;
}
