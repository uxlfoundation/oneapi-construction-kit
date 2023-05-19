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

// REQUIRES: double

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__attribute__((reqd_work_group_size(16, 1, 1)))
__kernel void double_sincos_local(__global double* in,
                                  __global double* out_sin,
                                  __global double* out_cos) {
  size_t id = get_global_id(0);
  size_t lid = get_local_id(0);

  __local double scratch[16];
  __local double *result = scratch + lid;
  out_sin[id] = sincos(in[id], result);
  out_cos[id] = result[0];
}
