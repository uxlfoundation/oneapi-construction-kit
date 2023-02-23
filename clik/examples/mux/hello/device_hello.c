// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "kernel_if.h"

__kernel void hello_mux(exec_state_t *item) {
  item->printf("Hello from ComputeMux! tid=%d, lid=%d, gid=%d\n",
               get_global_id(0, item), get_local_id(0, item),
               get_group_id(0, item));
}

// Execute the kernel once for each work-group contained in a work-slice.
// This function is called on each hardware thread of the device and there are
// as many slices as hardware threads.
int kernel_main(const void *args, exec_state_t *ctx) {
  ctx->local_id[0] = ctx->thread_id;
  for (uint i = 0; i < ctx->num_groups[0]; i++) {
    ctx->group_id[0] = i;
    hello_mux(ctx);
  }
  return 0;
}
