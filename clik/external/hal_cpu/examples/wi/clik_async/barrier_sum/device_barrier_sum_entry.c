// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "device_barrier_sum.h"

// Execute the kernel once for each work-group. This function is called on each
// hardware thread of the device. Together, all hardware threads on the device
// execute the same work-group. The N-D range can also be divided into slices in
// order to have more control over how work-groups are mapped to hardware
// threads.
void kernel_main(const barrier_sum_args *args, exec_state_t *ctx) {
  wg_info_t *wg = &ctx->wg;
  ctx->local_id[0] = ctx->thread_id;
  for (uint i = 0; i < wg->num_groups[0]; i++) {
    wg->group_id[0] = i;
    barrier_sum(args->src, args->dst, args->src_tile, ctx);

    // When local memory is used, a barrier is needed between work-groups to
    // ensure that all work-items in the group have finished executing before
    // starting the next group. Otherwise, a work-item from the 'next' group
    // might overwrite the data used by a work-item from the 'previous' group.
    barrier(ctx);
  }
}
