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

#include "device_matrix_multiply.h"

// Execute the kernel once for each work-item contained in the work-group
// specified by the work-group information. This function is called once per
// work-group in the N-D range. It can be called on different hardware threads,
// however different threads execute separate work-groups.
void kernel_main(uint64_t instance_id, uint64_t slice_id,
                 const matrix_multiply_args *args, wg_info_t *wg) {
  exec_state_t *ctx = get_context(wg);
  wg->group_id[0] = instance_id;
  wg->group_id[1] = slice_id;
  for (uint i = 0; i < wg->local_size[0]; i++) {
    ctx->local_id[0] = i;
    for (uint j = 0; j < wg->local_size[1]; j++) {
      ctx->local_id[1] = j;
      matrix_multiply(args->a, args->b, args->c, args->m, ctx);
    }
  }
}
