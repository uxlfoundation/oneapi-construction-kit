// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "device_vector_add.h"

// Carry out the computation for one work-item.
__kernel void vector_add(__global uint *src1, __global uint *src2,
                         __global uint *dst, exec_state_t *item) {
  uint tid = get_global_id(0, item);
  dst[tid] = src1[tid] + src2[tid];
}
