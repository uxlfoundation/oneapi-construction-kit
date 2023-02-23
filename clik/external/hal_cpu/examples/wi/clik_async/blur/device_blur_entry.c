// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "device_blur.h"

void copy_and_pad_hor_main(const copy_and_pad_hor_args *args,
                           exec_state_t *ctx) {
  wg_info_t *wg = &ctx->wg;
  ctx->local_id[0] = ctx->thread_id;
  for (uint i = 0; i < wg->num_groups[0]; i++) {
    wg->group_id[0] = i;
    copy_and_pad_hor(args->src, args->dst, ctx);
  }
}

void pad_vert_main(const pad_vert_args *args, exec_state_t *ctx) {
  wg_info_t *wg = &ctx->wg;
  ctx->local_id[0] = ctx->thread_id;
  for (uint i = 0; i < wg->num_groups[0]; i++) {
    wg->group_id[0] = i;
    pad_vert(args->buf, ctx);
  }
}

void blur_main(const blur_args *args, exec_state_t *ctx) {
  wg_info_t *wg = &ctx->wg;
  ctx->local_id[0] = ctx->thread_id;
  ctx->local_id[1] = 0;
  for (uint j = 0; j < wg->num_groups[1]; j++) {
    wg->group_id[1] = j;
    for (uint i = 0; i < wg->num_groups[0]; i++) {
      wg->group_id[0] = i;
      blur(args->src, args->dst, ctx);
    }
  }
}
