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

__kernel void wait_event_is_barrier(__local int *tmpA, __local int *tmpB, __global const int *A,
                                    __global const int *B, __global int *C) {
  size_t lid = get_local_id(0);
  size_t group = get_group_id(0);
  size_t size = get_local_size(0);
  event_t event;

  // Set up a state where even if wait_group_events does not behave like a
  // barrier we have an explicit barrier, so tmpA will be filled from A.
  event = async_work_group_copy(tmpA, &A[group * size], size, 0);
  wait_group_events(1, &event);
  barrier(CLK_LOCAL_MEM_FENCE);

  // Now, setup a state where tmpA gets overwritten with the state of B, have no
  // explicit barrier (as it should not be required, the wait_group_events
  // should be enough).
  event = async_work_group_copy(tmpA, &B[group * size], size, 0);
  wait_group_events(1, &event);

  // If wait_group_events is insufficiently strict then tmpA may now contain
  // either the contents of A or B, or a mix.  Copy/rotate it to tmpB for
  // reference and have a barrier to ensure we're done copying.  Note that it
  // is quite hard with ComputeAorta host to have the "wrong" data be in tmpA,
  // this is because if all work items redundantly copy all data for
  // async_work_group_copy then tmpA will always be valid at this point by
  // sheer brute force, if a single work item does it the obvious item to do
  // the work is <0,0,0>, which happens to run first -- but if you change the
  // implementation of async_work_group_copy to have the last work item do the
  // work then tmpA does not contain the correct data, and hardware that has a
  // true DMA unit could effectively be operating in this manner.
  tmpB[lid] = tmpA[(lid+1) % size];
  barrier(CLK_LOCAL_MEM_FENCE);

  // Copy tmpB out to C, so that we can test that it contains the contents of B
  // outside of the kernel.
  event = async_work_group_copy(&C[group * size], tmpB, size, 0);
  wait_group_events(1, &event);
}
