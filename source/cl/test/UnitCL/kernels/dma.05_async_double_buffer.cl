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

// Perform a vector addition using a double buffer async copy.  I.e. load into
// one buffer while calculating using another, then flip.  The output buffers
// are also double buffered so as the next iteration isn't immediately waiting
// for the output buffer to be available again.
//
//  ITERATION   -1  0   1   2   3   4   ...   N
//  LOAD        #0  #1  #2  #3  #4  #5  ...
//  CALC            #0  #1  #2  #3  #4  ...   #N
//  STORE           #0  #1  #2  #3  #4  ...   #N
__kernel void async_double_buffer(__local int *tmpA1, __local int *tmpA2,
                                  __local int *tmpB1, __local int *tmpB2,
                                  __local int *tmpC1, __local int *tmpC2,
                                  __global int *A, __global int *B,
                                  __global int *C, int iterations) {
  size_t lid = get_local_id(0);
  size_t group = get_group_id(0);
  size_t size = get_local_size(0);
  size_t global_size = get_global_size(0);

  // We have six temporary buffers, so we have six slots for events to keep
  // track of whether it is safe to read-from or write to each slot yet.
  enum { A1 = 0, B1, A2, B2, C1, C2, NUM_EVENTS };
  event_t events[NUM_EVENTS];

  // Start copying the first set of input data in immediately.
  events[A1] = async_work_group_copy(tmpA1, &A[group * size], size, 0);
  events[B1] = async_work_group_copy(tmpB1, &B[group * size], size, 0);

  for (int i = 0; i < iterations; i++) {
    bool even = (i % 2) == 0;

    // Trigger the copy that will be used in the next iteration, but only if
    // there will be a next iteration.
    if (i < (iterations - 1)) {
      events[even ? A2 : A1] = async_work_group_copy(
          even ? tmpA2 : tmpA1, &A[global_size * (i + 1) + group * size], size,
          0);
      events[even ? B2 : B1] = async_work_group_copy(
          even ? tmpB2 : tmpB1, &B[global_size * (i + 1) + group * size], size,
          0);
    }

    // Wait on the writes from two iterations ago to complete (before we
    // overwrite the data being copied), but only if there were writes two
    // iterations ago.
    if (i >= 2) {
      wait_group_events(1, &events[even ? C1 : C2]);
      barrier(CLK_LOCAL_MEM_FENCE);
    }

    // Wait on the reads triggered in the previous iteration to complete.
    wait_group_events(2, &events[even ? A1 : A2]);
    barrier(CLK_LOCAL_MEM_FENCE);

    // Perform the calculation and then wait for all work items to finish this
    // so that all workitems have the same state when entering the async copy.
    if (even) {
      tmpC1[lid] = tmpA1[lid] + tmpB1[lid];
    } else {
      tmpC2[lid] = tmpA2[lid] + tmpB2[lid];
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    // The current result has now been calculated, so copy it out.
    events[even ? C1 : C2] = async_work_group_copy(
        &C[global_size * i + group * size], even ? tmpC1 : tmpC2, size, 0);
  }

  // Wait on the writes of the last two iterations of the loop to complete.
  wait_group_events(2, &(events[C1]));
}
