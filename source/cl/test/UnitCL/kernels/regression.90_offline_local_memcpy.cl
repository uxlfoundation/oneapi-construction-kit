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

// `tmp` must be in __local memory, it can be either passed as a parameter or
// be statically sized inside the kernel, but it must be __local not __global
// for the segfault to occur.
__kernel void offline_local_memcpy(__local int *tmp, __global int *out) {
  size_t gid = get_global_id(0);
  size_t lid = get_local_id(0);
  size_t size = get_local_size(0);

  // We must save `gid` to `tmp[lid]`.  Simply using `gid` later on is not
  // enough to cause the segfault.  As mentioned above it is important that
  // `tmp` is in __local memory.
  tmp[lid] = (int)gid;

  // The barrier doesn't actually have anything to do with the segfault, but is
  // required to make the test semantically correct.
  barrier(CLK_LOCAL_MEM_FENCE);

  // This must be a loop of iteration length `size`. The use of 'size' needs to
  // be complex enough that it cannot be optimized out, this will result in a
  // memcpy being emitted.
  if (gid == 0) {
    for (size_t i = 0; i < size; i++) {
      out[i] = tmp[i];
    }
  }
}
