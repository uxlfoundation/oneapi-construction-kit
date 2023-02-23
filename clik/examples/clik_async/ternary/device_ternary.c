// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "device_ternary.h"

// Carry out the computation for one work-item.
__kernel void ternary(__global int *in1, int bias, __global int *out,
                      int trueVal, int falseVal, exec_state_t *item) {
  uint tid = get_global_id(0, item);
  out[tid] = (in1[tid] ? trueVal : falseVal) + bias;
}
