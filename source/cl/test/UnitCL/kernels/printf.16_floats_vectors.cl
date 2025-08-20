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

// SPIRV OPTIONS: -Wno-format
// DEFINITIONS: -D NUM_INPUTS=16

#ifndef NUM_INPUTS
#define NUM_INPUTS 1
#endif

kernel void floats_vectors(global float2* in, global float2* out) {
  size_t gid = get_global_id(0);
  out[gid] = in[gid] * in[gid];
  int i;
  // We are only printing from one workitem to make sure the order remains
  // constant
  if (gid == 0) {
    for (i = 0; i < NUM_INPUTS; ++i) {
      printf("%#16.1v2hlA\n", in[i] * in[i]);
    }
  }
}
