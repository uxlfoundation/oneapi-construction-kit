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

kernel void memop_loop_dep(global int *in, global int *out, int i, int e) {
  size_t gid = get_global_id(0);
  // i = 0, e = 1, for one iteration
  for (; i < e; i++) {
    int4 in4 = vload4(gid, in);
    vstore4(in4, gid, out);
    // this will never be executed, but at the stage that we run vecz it will
    // not be optimized away either
    if (in4.s0 && !gid) {
      while (gid)
        ;
    }
  }
}
