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

#include "device_matrix_multiply_tiled.h"

__kernel void matrix_multiply(__global float *a, __global float *b,
                              __global float *c, uint m, exec_state_t *ctx) {
  uint col = get_global_id(0, ctx);
  uint row = get_global_id(1, ctx);
  uint lx = get_local_id(0, ctx);
  uint ly = get_local_id(1, ctx);

  __local_variable float tile_a[TS * TS];
  __local_variable float tile_b[TS * TS];
  float sum = 0.0f;
  for (int i = 0; i < m; i += TS) {
    // Load one tile from A and B into local memory.
    tile_a[(ly * TS) + lx] = a[(row * m) + i + lx];
    tile_b[(ly * TS) + lx] = b[((i + ly) * m) + col];
    barrier(ctx);

    // Compute the partial sum for the loaded tile.
    for (int j = 0; j < TS; j++) {
      sum += tile_a[(ly * TS) + j] * tile_b[(j * TS) + lx];
    }

    // Wait for all items to be finished before computing the next tile.
    barrier(ctx);
  }
  c[(row * m) + col] = sum;
}
