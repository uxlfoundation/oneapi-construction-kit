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

#ifndef _CLIK_EXAMPLES_CLIK_ASYNC_VECTOR_ADD_WFV_H
#define _CLIK_EXAMPLES_CLIK_ASYNC_VECTOR_ADD_WFV_H

#include "kernel_if.h"
#if defined(__riscv_vector)
#include <riscv_vector.h>
#endif

__kernel void vector_add(__global uint *src1, __global uint *src2,
                         __global uint *dst, exec_state_t *item);

#if defined(__riscv_vector)
// Carry out the computation for VF items using scalable vector instructions.
// The caller is responsible for ensuring that `vsetvl_e32m1(vf) -> vf`.
__kernel void vector_add_rvv(__global uint *src1, __global uint *src2,
                             __global uint *dst, uint vf, exec_state_t *item);
#endif

typedef struct {
  __global uint *src1;
  __global uint *src2;
  __global uint *dst;
} vector_add_wfv_args;

#endif  // _CLIK_EXAMPLES_CLIK_ASYNC_VECTOR_ADD_WFV_H
