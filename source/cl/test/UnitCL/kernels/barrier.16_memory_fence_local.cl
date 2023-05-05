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
//
// TODO: CA-1830, FENCE_OP is parameterised for the online case. We need to set this up
// for the offline case as well.
// DEFINITIONS: "-DFENCE_OP=mem_fence"

__kernel void memory_fence_local(__global int* in, __local int* tmp,
                                 __global int* out) {
  size_t gid = get_global_id(0);
  size_t lid = get_local_id(0);
  tmp[lid] = in[gid];

  // This fence basically doesn't affect the test, but mem_fence is so
  // toothless that it is very hard to write a test that would fail without a
  // fence but pass with.  Having it here does at least exercise compiler
  // support for a fence though.
  FENCE_OP(CLK_LOCAL_MEM_FENCE);

  out[gid] = tmp[lid];
}
