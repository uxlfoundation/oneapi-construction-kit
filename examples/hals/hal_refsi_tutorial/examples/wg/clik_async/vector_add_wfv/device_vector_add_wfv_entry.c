// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "device_vector_add_wfv.h"

// Execute the kernel once for each work-item contained in the work-group
// specified by the work-group information. This function is called once per
// work-group in the N-D range. It can be called on different hardware threads,
// however different threads execute separate work-groups.
void kernel_main(uint64_t instance_id, uint64_t slice_id,
                 const vector_add_wfv_args *args, wg_info_t *wg) {
  exec_state_t *ctx = get_context(wg);
  if (instance_id == 0) {
    print(ctx,
          "Running kernel 'vector_add' (generic version). "
          "Total groups: %d\n",
          wg->num_groups[0]);
  }
  wg->group_id[0] = instance_id;
  for (uint i = 0; i < wg->local_size[0]; i++) {
    ctx->local_id[0] = i;
    vector_add(args->src1, args->src2, args->dst, ctx);
  }
}
