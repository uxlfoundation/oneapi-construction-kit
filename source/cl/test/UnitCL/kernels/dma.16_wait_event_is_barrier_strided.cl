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

__kernel void wait_event_is_barrier_strided(__local int *tmp,
                                            __global const int *in,
                                            __global int *out) {
  size_t lid = get_local_id(0);
  size_t gid = get_global_id(0);
  size_t group = get_group_id(0);
  size_t size = get_local_size(0);
  event_t event;

  // Write into the local temporary buffer.
  tmp[lid] = in[gid];

  // Barrier so we know tmp is filled.
  barrier(CLK_LOCAL_MEM_FENCE);

  // Now do a scatter into the output buffer.
  event = async_work_group_strided_copy(&out[group * size], tmp, size, 1, 0);
  wait_group_events(1, &event);

  // Increment the output buffer. If the wait_group_events is a nop then this
  // might catch it if async_workgroup_strided_copy didn't run on the first
  // thread to execute the builtin only.
  out[gid] += 1;
}
