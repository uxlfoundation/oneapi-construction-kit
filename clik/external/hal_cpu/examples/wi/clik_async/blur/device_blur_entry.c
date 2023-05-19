// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
