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

kernel void sibling_loops(global int *srcE, global int *srcO, global int *dst) {
  size_t x = get_global_id(0);
  size_t n = get_global_size(0);
  int sum = 0;
  for (size_t i = 0; i <= x; i++) {
    int val;
    if (i & 1)
      val = srcO[i] * 2;
    else
      val = srcE[i] * 3;
    sum += val;
  }
  for (size_t i = x + 1; i < n; i++) {
    int val;
    if (i & 1)
      val = srcE[i] * -5;
    else
      val = srcO[i] * 17;
    sum += val;
  }
  dst[x] = sum;
}
