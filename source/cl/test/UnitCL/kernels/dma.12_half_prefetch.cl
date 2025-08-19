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

// REQUIRES: half; parameters;
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef TYPE
#define TYPE half
#endif

__kernel void half_prefetch(__global TYPE *A, __global TYPE *B,
                            __global TYPE *C) {
  size_t gid = get_global_id(0);
  size_t group = get_group_id(0);
  size_t size = get_local_size(0);

  // Prefetch the input data.
  prefetch(&A[group * size], size);
  prefetch(&B[group * size], size);

  C[gid] = fmax(A[gid], B[gid]);
}
