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

kernel void async_copy(const __global uint *in, __local uint* buffer,
                       __global uint *out) {
  size_t group = get_group_id(0);
  size_t size = get_local_size(0);

  event_t e = async_work_group_strided_copy(buffer, &in[group * size], size, 1, 0);
  wait_group_events(1, &e);
  e = async_work_group_strided_copy(&out[group * size], (const __local uint*)buffer, size, 1, 0);
  wait_group_events(1, &e);
}
