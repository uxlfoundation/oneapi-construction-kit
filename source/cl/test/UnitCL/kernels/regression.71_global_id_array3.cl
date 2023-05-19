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

typedef struct __attribute__((packed)) {
  uint id[3];
} id_data;

__kernel void global_id_array3(__global uint* size, __global id_data* out) {
  size_t gid = get_global_id(0);

  size[gid] = sizeof(id_data);

  for (int i = 0; i < 3; i++) {
    out[gid].id[i] = (uint)get_global_id(i);
  }
}
