// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "device_vector_add_wfv.h"

#if defined(__riscv_vector)
// Carry out the computation for VF items using scalable vector instructions.
// The caller is responsible for ensuring that `vsetvl_e32m1(vf) -> vf`.
__kernel void vector_add_rvv(__global uint *src1, __global uint *src2,
                             __global uint *dst, uint vf, exec_state_t *item) {
  uint tid = get_global_id(0, item);
  uint index = tid * vf;

  vsetvl_e32m1(vf);
  vint32m1_t x = vle32_v_i32m1(&src1[index]);
  vint32m1_t y = vle32_v_i32m1(&src2[index]);
  vint32m1_t z = vadd_vv_i32m1(x, y);
  vse32_v_i32m1(&dst[index], z);
}
#endif

// Carry out the computation for one work-item.
__kernel void vector_add(__global uint *src1, __global uint *src2,
                         __global uint *dst, exec_state_t *item) {
  uint tid = get_global_id(0, item);
  dst[tid] = src1[tid] + src2[tid];
}
