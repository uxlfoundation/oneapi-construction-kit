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

// This regression test is designed to catch the case that implementations of
// async_work_group_copy respect the padding on vectors with three elements
// mandated by the OpenCL C spec.

kernel void async_work_group_copy_int3(global int3 *in, global int3 *out,
                                       local int3 *tmp) {
  size_t group_id = get_group_id(0);
  size_t size = get_local_size(0);

  // Copy into the local buffer.
  event_t first_event =
      async_work_group_copy(tmp, &in[group_id * size], size, 0);
  wait_group_events(1, &first_event);

  // Then copy straight back out.
  event_t second_event =
      async_work_group_copy(&out[group_id * size], tmp, size, 0);
  wait_group_events(1, &second_event);
}
