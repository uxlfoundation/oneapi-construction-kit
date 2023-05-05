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
// CL_STD: 3.0
__kernel void fetch_local_and_check_return_uint(
    __global uint *input_buffer, __global uint *output_buffer,
    volatile __local atomic_uint *local_buffer) {
  uint gid = get_global_id(0);
  uint lid = get_local_id(0);

  atomic_init(local_buffer + lid, input_buffer[gid]);

  output_buffer[gid] = atomic_fetch_and_explicit(
      local_buffer + lid, 0, memory_order_relaxed, memory_scope_work_item);
}
