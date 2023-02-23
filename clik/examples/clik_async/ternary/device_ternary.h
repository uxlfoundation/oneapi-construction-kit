// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _CLIK_EXAMPLES_CLIK_ASYNC_TERNARY_H
#define _CLIK_EXAMPLES_CLIK_ASYNC_TERNARY_H

#include "kernel_if.h"

__kernel void ternary(__global int *in1, int bias, __global int *out,
                      int trueVal, int falseVal, exec_state_t *item);

typedef struct {
  __global int *in1;
  int bias;
  // On 64-bit archs there will be 4 bytes of padding here.
  __global int *out;
  int trueVal;
  int falseVal;
} ternary_args;

#endif  // _CLIK_EXAMPLES_CLIK_ASYNC_TERNARY_H
