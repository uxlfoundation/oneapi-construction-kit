// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "kernel_if.h"

// Carry out the computation for one work-item.
__kernel void vector_add(__global uint *src1, __global uint *src2,
                         __global uint *dst, exec_state_t *item) {
  uint tid = get_global_id(0, item);
  dst[tid] = src1[tid] + src2[tid];
}

typedef struct {
  __global uint *src1;
  __global uint *src2;
  __global uint *dst;
} vector_add_args;

// Execute the kernel once for each work-group contained in a work-slice.
// This function is called on each hardware thread of the device and there are
// as many slices as hardware threads.
int kernel_main(const vector_add_args *args, exec_state_t *ctx) {
  ctx->local_id[0] = ctx->thread_id;
  for (uint i = 0; i < ctx->num_groups[0]; i++) {
    ctx->group_id[0] = i;
    vector_add(args->src1, args->src2, args->dst, ctx);
  }
  return 0;
}
