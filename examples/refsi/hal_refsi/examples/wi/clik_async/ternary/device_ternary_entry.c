// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "device_ternary.h"

// Execute the kernel once for each work-group. This function is called on each
// hardware thread of the device. Together, all hardware threads on the device
// execute the same work-group. The N-D range can also be divided into slices in
// order to have more control over how work-groups are mapped to hardware
// threads.
void kernel_main(const ternary_args *args, exec_state_t *ctx) {
  wg_info_t *wg = &ctx->wg;
  ctx->local_id[0] = ctx->thread_id;
  for (uint i = 0; i < wg->num_groups[0]; i++) {
    wg->group_id[0] = i;
    ternary(args->in1, args->bias, args->out, args->trueVal, args->falseVal,
            ctx);
  }
}
