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

// The purpose of this test is to make sure the uniform value analysis considers
// the condition `i == 4` as being varying since its value can be modified in a
// divergent block.
__kernel void partial_linearization_varying_uniform_condition(__global int *out, int n) {
  int x = get_global_id(0);
  int i = n;
  int ret = 0;

  if (x == 2) {
    for (int j = 0; j <= n + 1; ++j) { i += n; }
  }

  if (i == 4) {
    for (int j = 0; j <= n; ++j) { ret += j; }
  }
  out[x] = ret;
}
