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
__kernel void store_global_long(__global long *input_buffer,
                                volatile __global atomic_long *output_buffer) {
  uint gid = get_global_id(0);
  switch (gid & 3) {
    case 0:
      atomic_store(output_buffer + gid, input_buffer[gid]);
      break;
    case 1:
      atomic_store_explicit(output_buffer + gid, input_buffer[gid],
                            memory_order_relaxed);
      break;
    default:
      atomic_store_explicit(output_buffer + gid, input_buffer[gid],
                            memory_order_relaxed, memory_scope_work_item);
      break;
  }
}
