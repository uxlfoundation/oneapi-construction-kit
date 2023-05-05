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
#define TRIPS 256

__kernel void sum_static_trip(__global int *in1, __global int *in2,
                              __global int *out) {
  size_t tid = get_global_id(0);

  int sum = 0;
  for (int i = 0; i < TRIPS; i++) {
    sum += (in1[i] * i) + in2[i];
  }

  out[tid] = sum;
}
