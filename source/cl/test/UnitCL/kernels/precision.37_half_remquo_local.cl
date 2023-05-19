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

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef FLOAT_TYPE
#define FLOAT_TYPE half
#endif

#ifndef INT_TYPE
#define INT_TYPE int
#endif

__attribute__((reqd_work_group_size(4, 1, 1)))
__kernel void half_remquo_local(__global FLOAT_TYPE* x,
                                __global FLOAT_TYPE* y,
                                __global FLOAT_TYPE* out,
                                __global INT_TYPE* out_int) {
  size_t tid = get_global_id(0);
  size_t lid = get_local_id(0);
  __local INT_TYPE buffer[4];
  __local INT_TYPE *result = buffer + lid;
  out[tid] = remquo(x[tid], y[tid], result);
  out_int[tid] = result[0];
}
