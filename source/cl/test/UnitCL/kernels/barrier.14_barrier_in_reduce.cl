// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

__kernel void barrier_in_reduce(__global const int *src, __global int *dst,
                                __local int *block) {
  size_t lid = get_local_id(0);
  size_t gid = get_global_id(0);
  size_t offset = get_local_size(0) >> 1;

  block[lid] = src[gid];

  do {
    barrier(CLK_LOCAL_MEM_FENCE);

    if (lid < offset) {
      int accum = block[lid];
      accum += block[lid + offset];
      block[lid] = accum;
    }

    offset >>= 1;
  } while (offset > 0);

  // Note, no barrier here because only thread 0 wrote to address block[0], and
  // only thread 0 will read block[0] below.  This lack of barrier is
  // essentially the point of the test.

  if (lid == 0) {
    dst[get_group_id(0)] = block[0];
  }
}
