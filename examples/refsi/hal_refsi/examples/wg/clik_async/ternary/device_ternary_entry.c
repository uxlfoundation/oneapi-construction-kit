// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "device_ternary.h"

// Execute the kernel once for each work-item contained in the work-group
// specified by the work-group information. This function is called once per
// work-group in the N-D range. It can be called on different hardware threads,
// however different threads execute separate work-groups.
void kernel_main(const ternary_args *args, wg_info_t *wg) {
  exec_state_t *ctx = get_context(wg);
  for (uint i = 0; i < wg->local_size[0]; i++) {
    ctx->local_id[0] = i;
    ternary(args->in1, args->bias, args->out, args->trueVal, args->falseVal,
            ctx);
  }
}
