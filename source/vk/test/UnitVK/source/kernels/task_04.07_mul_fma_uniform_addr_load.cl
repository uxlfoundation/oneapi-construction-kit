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

__kernel void mul_fma_uniform_addr_load(__global int *in, __global int *out1,
                                        __global int *out2) {
  // Uniform-offset addressing.
  size_t lid = get_local_id(0);
  size_t lsize = get_local_size(0);
  int base = get_global_offset(0) + (get_group_id(0) * lsize);
  size_t tid = base + lid;

  // Layout for in: in1[0] in2[0] in3[0] in1[1] in2[1] in3[1]
  int indexIn1 = (base * 3) + (lsize * 0) + lid;
  int indexIn2 = (base * 3) + (lsize * 1) + lid;
  int indexIn3 = (base * 3) + (lsize * 2) + lid;

  int a = in[indexIn1];
  int b = in[indexIn2];
  int c = in[indexIn3];
  int temp = a * b;

  out1[tid] = temp;
  out2[tid] = temp + c;
}
