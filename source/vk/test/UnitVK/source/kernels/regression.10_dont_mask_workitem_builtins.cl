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

__kernel void dont_mask_workitem_builtins(__constant int* in,
                                          __global int* out) {
  int lid = get_local_id(0);

  if (lid > 0) {
    int gid = get_global_id(0);
    out[gid] = in[gid];
  } else {
    // don't use get_global_id again, to prevent the compiler for lifting
    // them outside the if/else block
    int gid = lid + (get_local_size(0) * get_group_id(0));
    out[gid] = 42;
  }
}
