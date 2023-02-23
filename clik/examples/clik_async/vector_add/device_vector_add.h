// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _CLIK_EXAMPLES_CLIK_ASYNC_VECTOR_ADD_H
#define _CLIK_EXAMPLES_CLIK_ASYNC_VECTOR_ADD_H

#include "kernel_if.h"

__kernel void vector_add(__global uint *src1, __global uint *src2,
                         __global uint *dst, exec_state_t *item);

typedef struct {
  __global uint *src1;
  __global uint *src2;
  __global uint *dst;
} vector_add_args;

#endif  // _CLIK_EXAMPLES_CLIK_ASYNC_VECTOR_ADD_H
