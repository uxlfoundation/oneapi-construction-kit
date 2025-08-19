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
// CL_STD: 3.0
__kernel void fetch_local_sub_int(__global int *input_buffer,
                                  __global int *output_buffer,
                                  __global int *initial_values,
                                  volatile __local atomic_int *local_buffer) {
  uint gid = get_global_id(0);
  uint lid = get_local_id(0);
  uint wgid = get_group_id(0);

  if (lid == 0) {
    atomic_init(local_buffer + lid, initial_values[wgid]);
  }
  work_group_barrier(CLK_LOCAL_MEM_FENCE);
  atomic_fetch_sub_explicit(local_buffer, input_buffer[gid],
                            memory_order_relaxed, memory_scope_work_item);
  work_group_barrier(CLK_LOCAL_MEM_FENCE);
  if (lid == 0) {
    output_buffer[wgid] = atomic_load_explicit(
        local_buffer, memory_order_relaxed, memory_scope_work_item);
  }
}
