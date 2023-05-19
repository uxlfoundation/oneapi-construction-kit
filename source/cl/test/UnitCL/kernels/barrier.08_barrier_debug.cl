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

// Contrived program for testing debugability of kernels with barriers,
// should be compiled with '-g' & '-cl-opt-disable'.

// This function will be inlined into the kernel. Included to test
// the debugger can set breakpoints & step into it despite being inlined.
int helper_multiply(int A, int N) {
  int sum = 0;
  for (int i = 0; i < N; i++) {
    // Barrier in loop to test debuggability of phi node transformations.
    barrier(CLK_LOCAL_MEM_FENCE);  // Barrier B
    sum += A;
  }
  return sum;
}

void barrier_debug2(__global const int* input, __global int* output) {
  // 'global_id' variable is live throughout kernel
  size_t global_id = get_global_id(0);

  // 'local_id' is not used past barrier 'A' so would not normally be included
  // in any liveness information. Test the debugger can still print the correct
  // value throughout the kernel.
  size_t local_id = get_local_id(0);

  // This if condition is to add an extra layer of lexical scope to the debug
  // info and test phi nodes.
  if (global_id < get_global_size(0)) {
    size_t multiplied = global_id * local_id;

    barrier(CLK_GLOBAL_MEM_FENCE);  // Barrier A

    // 'temp_val' is only used between barriers B and C, test
    // the debugger can still print it's value after barrier C.
    int temp_val = helper_multiply(input[global_id], 2);
    int result = temp_val + multiplied;

    barrier(CLK_LOCAL_MEM_FENCE);  // Barrier C

    output[global_id] = result;
  }
}

__kernel void barrier_debug(__global const int* input, __global int* output) {
  barrier_debug2(input, output);
}
