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

kernel void add3(global int3 *in1, global int3 *in2, global int3 *out) {
  size_t tid = get_global_id(0);

  int3 a = vload3(0, (global int *)&in1[tid]);
  int3 b = vload3(0, (global int *)&in2[tid]);
  int3 c = a + b;

  vstore3(c, 0, (global int *)&out[tid]);
}
