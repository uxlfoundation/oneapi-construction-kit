// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "device_matrix_multiply.h"

__kernel void matrix_multiply(__global float *a, __global float *b,
                              __global float *c, uint m, exec_state_t *item) {
  uint col = get_global_id(0, item);
  uint row = get_global_id(1, item);
  float sum = 0.0f;
  for (int i = 0; i < m; i++) {
    sum += a[(row * m) + i] * b[(i * m) + col];
  }
  c[(row * m) + col] = sum;
}
