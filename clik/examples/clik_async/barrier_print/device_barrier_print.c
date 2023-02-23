// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "device_barrier_print.h"

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

__kernel void barrier_print(exec_state_t *ctx) {
  uint tid = get_global_id(0, ctx);
  print(ctx, "Kernel part A (tid = %d)\n", tid);
  barrier(ctx);
  print(ctx, "Kernel part B (tid = %d)\n", tid);
}
