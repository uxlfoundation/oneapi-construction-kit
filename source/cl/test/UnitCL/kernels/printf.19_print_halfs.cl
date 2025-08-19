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

// REQUIRES: half
// SPIRV OPTIONS: -Xclang;-cl-ext=-cl_khr_fp64,+cl_khr_fp16;-Wno-format
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

__kernel void print_halfs(__global half* a, __global half* b) {
  half2 input = (half2)(a[0], b[0]);
  printf("input: (%a, %a)\n", input.x, input.y);
}
