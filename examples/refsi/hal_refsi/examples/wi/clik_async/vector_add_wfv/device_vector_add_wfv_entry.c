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

#include "device_vector_add_wfv.h"

// Execute the kernel once for each work-group. This function is called on each
// hardware thread of the device. Together, all hardware threads on the device
// execute the same work-group. The N-D range can also be divided into slices in
// order to have more control over how work-groups are mapped to hardware
// threads.
void kernel_main(const vector_add_wfv_args *args, exec_state_t *ctx) {
  wg_info_t *wg = &ctx->wg;
#if defined(__riscv_vector)
  // Detect the size of vector registers, in bytes.
  vsetvlmax_e8m1();
  uint vlen = vreadvl();
  vsetvl_e8m1(0);
  if (ctx->thread_id == 0) {
    print(ctx, "RVV extension is supported. Vector register size: %d bits\n",
          vlen * 8);
  }

  // Try to find a suitable vectorization factor VF, based on the vector
  // register size and number of work-groups passed to the kernel.
  uint vf = vlen / sizeof(uint);
  while (vf > 1) {
    if ((wg->num_groups[0] % vf) == 0) {
      break;
    }
    vf--;
  }

  // Run the RVV kernel with the given vectorization factor VF, when suitable.
  if ((vf > 1) && (wg->num_groups[0] % vf) == 0) {
    // Reduce the number of total work-items based on how many array elements
    // (equal to VF) are processed by each work-item using RVV.
    wg->num_groups[0] /= vf;

    if (thread_id == 0) {
      print(ctx, "Running kernel 'vector_add_rvv' (vectorized version, VF: %d). "
            "Total groups: %d\n", vf, wg->num_groups[0]);
    }
    ctx->local_id[0] = thread_id;
    for (uint i = 0; i < wg->num_groups[0]; i++) {
      wg->group_id[0] = i;
      vector_add_rvv(args->src1, args->src2, args->dst, vf, ctx);
    }
    return;
  }

  // When a suitable vectorization factor could not be found, fall back to the
  // generic version of the vector addition kernel.
#endif

  if (ctx->thread_id == 0) {
    print(ctx, "Running kernel 'vector_add' (generic version). "
          "Total groups: %d\n", wg->num_groups[0]);
  }
  ctx->local_id[0] = ctx->thread_id;
  for (uint i = 0; i < wg->num_groups[0]; i++) {
    wg->group_id[0] = i;
    vector_add(args->src1, args->src2, args->dst, ctx);
  }
}
