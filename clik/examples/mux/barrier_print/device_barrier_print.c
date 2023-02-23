// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "kernel_if.h"

// In this example, the kernel has two computation steps represented by printf
// calls. A barrier is used to ensure step A has been performed by all
// work-items in the work-group before any work-item in the group starts step B.
//
// With a work-group size of four, the output of this example will look like:
//
// Kernel part A (tid = 0)
// Kernel part A (tid = 1)
// Kernel part A (tid = 2)
// Kernel part A (tid = 3)
// Kernel part B (tid = 0)
// Kernel part B (tid = 1)
// Kernel part B (tid = 2)
// Kernel part B (tid = 3)
//
// Without a barrier between the two steps, the output would look like:
//
// Kernel part A (tid = 0)
// Kernel part B (tid = 0)
// Kernel part A (tid = 1)
// Kernel part B (tid = 1)
// Kernel part A (tid = 2)
// Kernel part B (tid = 2)
// Kernel part A (tid = 3)
// Kernel part B (tid = 3)

__kernel void barrier_print(exec_state_t *item) {
  uint tid = get_global_id(0, item);
  item->printf("Kernel part A (tid = %d)\n", tid);
  item->barrier(item);
  item->printf("Kernel part B (tid = %d)\n", tid);
}

// Execute the kernel once for each work-group contained in a work-slice.
// This function is called on each hardware thread of the device and there are
// as many slices as hardware threads.
int kernel_main(const void *args, exec_state_t *ctx) {
  ctx->local_id[0] = ctx->thread_id;
  for (uint i = 0; i < ctx->num_groups[0]; i++) {
    ctx->group_id[0] = i;
    barrier_print(ctx);

    // Wait for all items in the work-group to have been executed before
    // executing the next work-group.
    ctx->barrier(ctx);
  }
  return 0;
}
