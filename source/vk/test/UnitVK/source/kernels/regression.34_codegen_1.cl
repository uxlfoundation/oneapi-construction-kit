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

__kernel void codegen_1(const __global int *in0, const __global int *in1,
                        const __global int *in2, __global int *out,
                        const __global int *size, int reps) {
  size_t gid = get_global_id(0);
  int sum = 0;

  for (size_t i = gid * reps; i < (gid + 1) * reps; i++) {
    if (i < size[0]) sum += in0[i];
    if (i < size[1]) sum += in1[i];
    if (i < size[2]) sum += in2[i];
  }

  out[gid] = sum;
}
