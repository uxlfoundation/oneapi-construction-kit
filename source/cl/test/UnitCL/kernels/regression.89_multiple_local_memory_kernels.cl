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

// DEFINITIONS: -DLOCAL_X=1

kernel void another_kernel(global int* src, global int* dst){
  __local int localBuff[LOCAL_X];

  int lid = get_local_id(0);
  int gid = get_global_id(0);
  int index = (gid * LOCAL_X) + lid;

  localBuff[lid] = src[index];

  barrier(CLK_LOCAL_MEM_FENCE);

  int result = 0;

  result = localBuff[lid];
  dst[index] = result;

}

kernel void multiple_local_memory_kernels(global int* src, global int* dst){
  __local int localBuff[LOCAL_X];

  int lid = get_local_id(0);
  int gid = get_global_id(0);
  int index = (gid * LOCAL_X) + lid;

  localBuff[lid] = src[index];

  barrier(CLK_LOCAL_MEM_FENCE);

  int result = 0;

  result = localBuff[lid];
  dst[index] = result;
}
