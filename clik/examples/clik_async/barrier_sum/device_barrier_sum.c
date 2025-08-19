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

#include "device_barrier_sum.h"

// In this example, each work-item in the kernel computes the sum of a few
// values. These values are the same for all items in the work-group. In order
// to reduce the number of memory operations, a small local array is used to
// share these values between the items in the group. Each item only needs to
// load one item from the source buffer, however a barrier is needed to ensure
// all values have been copied before computing the sum.

__kernel void barrier_sum(__global uint *src, __global uint *dst,
                          __local uint *src_tile, exec_state_t *ctx) {
  uint tid = get_global_id(0, ctx);

  // Copy values from the source buffer to a local tile.
  uint lid = get_local_id(0, ctx);
  src_tile[lid] = src[tid];

  // Wait for all items in the group to have finished copying values.
  barrier(ctx);

  // Sum values from the tile and write the result to the output buffer.
  uint sum = 0;
  for (uint i = 0; i < get_local_size(0, ctx); i++) {
    sum += src_tile[i];
  }
  dst[tid] = sum;
}
