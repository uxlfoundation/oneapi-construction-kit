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

__kernel void scan_fact(__global int *out, __global const int *in) {
  size_t lid = get_local_id(0);
  size_t gid = get_global_id(0);
  size_t group_size = get_local_size(0);

  __local int temp[8]; // length of global_size
  temp[2 * lid] = in[2 * gid];
  temp[2 * lid + 1] = in[2 * gid + 1];

  int second_in = temp[2 * lid + 1];
  for (size_t off = 1; off < (group_size * 2); off <<= 1) {
    barrier(CLK_LOCAL_MEM_FENCE);
    size_t i = lid * off * 2;
    if (i < group_size * 2) {
      temp[i + off * 2 - 1] = temp[i + off * 2 - 1] * temp[i + off - 1];
    }
  }

  if (lid == 0) {
    temp[group_size * 2 - 1] = 1;
  }

  for (size_t off = group_size; off > 0; off >>= 1) {
    barrier(CLK_LOCAL_MEM_FENCE);
    size_t i = lid * off * 2;
    if (i < group_size * 2) {
      int t = temp[i + off - 1];
      int u = temp[i + off * 2 - 1];
      temp[i + off - 1] = u;
      temp[i + off * 2 - 1] = t * u;
    }
  }

  barrier(CLK_LOCAL_MEM_FENCE);

  out[2 * gid] = temp[2 * lid + 1];

  if (lid == group_size - 1) {
    out[2 * gid + 1] = temp[2 * lid + 1] * second_in;
  } else {
    out[2 * gid + 1] = temp[2 * lid + 2];
  }
}
