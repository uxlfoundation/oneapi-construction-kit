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

// Set reqd_work_group_size to ensure that the segfault was not special to the
// local-size-unknown compile path (it was not).
__attribute__((reqd_work_group_size(17, 1, 1)))
// `tmp` must be in __local memory for the segfault to occur.
__kernel void
offline_local_memcpy_fixed(__local int *tmp, __global int *out) {
  for (size_t i = 0; i < 17; i++) {
    tmp[i] = i;
  }

  // The barrier doesn't actually have anything to do with the segfault, but is
  // required to make the test semantically correct (or not racy at least).
  barrier(CLK_LOCAL_MEM_FENCE);

  // This must be a loop must have a size of at least 17, most likely because
  // for 32-bit systems LLVM considers sizes of <= 16 to be "small" and emits
  // an inline memcpy.
  if (get_global_id(0) == 0) {
    for (size_t i = 0; i < 17; i++) {
      out[i] = tmp[i];
    }
  }
}
