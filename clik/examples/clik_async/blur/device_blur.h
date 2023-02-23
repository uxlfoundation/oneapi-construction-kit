// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _CLIK_EXAMPLES_CLIK_ASYNC_BLUR_H
#define _CLIK_EXAMPLES_CLIK_ASYNC_BLUR_H

#include "constants.h"
#include "kernel_if.h"

__kernel void copy_and_pad_hor(__global uint *src, __global uint *dst,
                               exec_state_t *item);
__kernel void pad_vert(__global uint *dst, exec_state_t *item);
__kernel void blur(__global uint *src, __global uint *dst, exec_state_t *item);

// Argument structs for kernels

typedef struct {
  __global uint *src;
  __global uint *dst;
} copy_and_pad_hor_args;

typedef struct {
  __global uint *buf;
} pad_vert_args;

typedef struct {
  __global uint *src;
  __global uint *dst;
} blur_args;

#endif  // _CLIK_EXAMPLES_CLIK_ASYNC_BLUR_H
