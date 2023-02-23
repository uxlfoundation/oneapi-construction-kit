// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
