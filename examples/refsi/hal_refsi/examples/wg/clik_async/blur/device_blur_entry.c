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

// Execute the kernel once for each work-item contained in the work-group
// specified by the work-group information. This function is called once per
// work-group in the N-D range. It can be called on different hardware threads,
// however different threads execute separate work-groups.
void copy_and_pad_hor_main(const copy_and_pad_hor_args *args, wg_info_t *wg) {
  exec_state_t *ctx = get_context(wg);
  for (uint i = 0; i < wg->local_size[0]; i++) {
    ctx->local_id[0] = i;
    copy_and_pad_hor(args->src, args->dst, ctx);
  }
}

void pad_vert_main(const pad_vert_args *args, wg_info_t *wg) {
  exec_state_t *ctx = get_context(wg);
  for (uint i = 0; i < wg->local_size[0]; i++) {
    ctx->local_id[0] = i;
    pad_vert(args->buf, ctx);
  }
}

void blur_main(const blur_args *args, wg_info_t *wg) {
  exec_state_t *ctx = get_context(wg);
  for (uint i = 0; i < wg->local_size[0]; i++) {
    ctx->local_id[0] = i;
    for (uint j = 0; j < wg->local_size[1]; j++) {
      ctx->local_id[1] = j;
      blur(args->src, args->dst, ctx);
    }
  }
}
