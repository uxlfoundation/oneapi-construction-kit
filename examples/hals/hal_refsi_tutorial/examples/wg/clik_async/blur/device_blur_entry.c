// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "device_blur.h"

// Execute the kernel once for each work-item contained in the work-group
// specified by the work-group information. This function is called once per
// work-group in the N-D range. It can be called on different hardware threads,
// however different threads execute separate work-groups.
void copy_and_pad_hor_main(uint64_t instance_id, uint64_t slice_id,
                           const copy_and_pad_hor_args *args, wg_info_t *wg) {
  exec_state_t *ctx = get_context(wg);
  wg->group_id[0] = instance_id;
  for (uint i = 0; i < wg->local_size[0]; i++) {
    ctx->local_id[0] = i;
    copy_and_pad_hor(args->src, args->dst, ctx);
  }
}

void pad_vert_main(uint64_t instance_id, uint64_t slice_id,
                   const pad_vert_args *args, wg_info_t *wg) {
  exec_state_t *ctx = get_context(wg);
  wg->group_id[0] = instance_id;
  for (uint i = 0; i < wg->local_size[0]; i++) {
    ctx->local_id[0] = i;
    pad_vert(args->buf, ctx);
  }
}

void blur_main(uint64_t instance_id, uint64_t slice_id, const blur_args *args,
               wg_info_t *wg) {
  exec_state_t *ctx = get_context(wg);
  wg->group_id[0] = instance_id;
  wg->group_id[1] = slice_id;
  for (uint i = 0; i < wg->local_size[0]; i++) {
    ctx->local_id[0] = i;
    for (uint j = 0; j < wg->local_size[1]; j++) {
      ctx->local_id[1] = j;
      blur(args->src, args->dst, ctx);
    }
  }
}
