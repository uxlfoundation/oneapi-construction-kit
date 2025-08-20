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

__kernel void mul_fma_uniform_addr_store(__global int *in1, __global int *in2,
                                         __global int *in3, __global int *out) {
  // Uniform-offset addressing.
  size_t lid = get_local_id(0);
  size_t lsize = get_local_size(0);
  int base = get_global_offset(0) + (get_group_id(0) * lsize);
  size_t tid = base + lid;

  // Layout for out: out1[0] out2[0] out1[1] out2[1]
  int indexOut1 = (base * 2) + (lsize * 0) + lid;
  int indexOut2 = (base * 2) + (lsize * 1) + lid;

  int a = in1[tid];
  int b = in2[tid];
  int c = in3[tid];
  int temp = a * b;

  out[indexOut1] = temp;
  out[indexOut2] = temp + c;
}
