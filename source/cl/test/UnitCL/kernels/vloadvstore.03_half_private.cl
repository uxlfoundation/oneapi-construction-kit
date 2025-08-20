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

// REQUIRES: half parameters
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

__kernel void half_private(__global half *input,
                           uint iterations,
                           __global half *output) {
  size_t gid = get_global_id(0);
  HALFN global_copy = LOADN(gid, input);  // vload from __global

  __private half private_array[ARRAY_LEN];
  for (uint i = 0; i < iterations; i++) {
    STOREN(global_copy, i, private_array);  // vstore to __private
  }

  bool identical = true;
  for (uint i = 0; i < iterations; i++) {
    HALFN private_copy = LOADN(i, private_array);  // vload from __private
    bool are_equal = all(isequal(global_copy, private_copy) ||
                         (isnan(global_copy) && isnan(private_copy)));
    identical = identical && are_equal;
  }

  if (identical) {
    STOREN(global_copy, gid, output);  // vstore to __global
  }
};
