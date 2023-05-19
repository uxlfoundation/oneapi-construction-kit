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
__kernel void flag_local_clear_set(__global int *out_buffer,
                                   volatile __local atomic_flag *local_buffer) {
  uint gid = get_global_id(0);
  uint lid = get_local_id(0);

  atomic_flag_clear_explicit(local_buffer + lid, memory_order_relaxed,
                             memory_scope_work_item);
  out_buffer[gid] = atomic_flag_test_and_set_explicit(
      local_buffer + lid, memory_order_relaxed, memory_scope_work_item);
}
