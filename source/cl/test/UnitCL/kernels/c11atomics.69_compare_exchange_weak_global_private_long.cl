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
__kernel void compare_exchange_weak_global_private_long(
    volatile __global atomic_long *atomic_buffer,
    __global long *expected_buffer, __global long *desired_buffer,
    int __global *bool_output_buffer) {
  int gid = get_global_id(0);
  int lid = get_global_id(0);

  long private_expected = expected_buffer[gid];

  bool_output_buffer[gid] = atomic_compare_exchange_weak_explicit(
      atomic_buffer + gid, &private_expected, desired_buffer[gid],
      memory_order_relaxed, memory_order_relaxed, memory_scope_work_item);

  expected_buffer[gid] = private_expected;
}
