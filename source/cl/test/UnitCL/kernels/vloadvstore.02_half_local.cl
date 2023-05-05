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

// REQUIRES: half parameters
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

__kernel void half_local(__global half *input,
                         __local half *scratch,
                         __global half *output) {
  size_t gid = get_global_id(0);
  HALFN global_copy = LOADN(gid, input);  // vload from __global

  size_t lid = get_local_id(0);
  STOREN(global_copy, lid, scratch);  // vstore to __local

  barrier(CLK_LOCAL_MEM_FENCE);

  HALFN local_copy = LOADN(lid, scratch);  // vload from __local
  STOREN(local_copy, gid, output);  // vstore to __global
};
