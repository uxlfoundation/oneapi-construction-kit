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

__kernel void wait_event_is_barrier_overwrite(__local int *tmp,
                                              __global const int *in,
                                              __global int *out) {
  size_t lid = get_local_id(0);
  size_t group = get_group_id(0);
  size_t size = get_local_size(0);
  event_t event;

  // Fill tmp from the input buffer via async_work_group_copy, but immediately
  // wait for it to complete.
  event = async_work_group_copy(tmp, &in[group * size], size, 0);
  wait_group_events(1, &event);

  // Now, increment this work item's value in tmp, and then have a barrier.
  // If, however, the wait_group_events above did not truly wait then it is
  // possible that the correct value was not yet in tmp, or that the correct
  // value is there but will be overwritten because the memory operation can
  // happen multiple times.  Note: at the time of writing this test the
  // multiple memory operation behavior was visible in ComputeAorta.
  tmp[lid] += 1;
  barrier(CLK_LOCAL_MEM_FENCE);

  // Now copy the data back out so that it can be verified externally.
  event = async_work_group_copy(&out[group * size], tmp, size, 0);
  wait_group_events(1, &event);
}
