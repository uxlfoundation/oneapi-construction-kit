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

#define ARRAY_SIZE 16

__kernel void barrier_add(__global int *in1, __global int *in2,
                          __global int *out) {
  size_t tid = get_global_id(0);
  size_t lid = get_local_id(0);
  __local volatile int temp[ARRAY_SIZE];
  temp[lid] = in1[tid] + in2[tid];
  barrier(CLK_LOCAL_MEM_FENCE);

  size_t lsize = get_local_size(0);
  size_t base = get_group_id(0) * lsize;
  int num_correct = 0;
  for (unsigned i = 0; i < lsize; i++) {
    int expected = in1[base + i] + in2[base + i];
    int actual = temp[i];
    num_correct += (expected == actual) ? 1 : 0;
  }
  out[tid] = (num_correct == lsize);
}
