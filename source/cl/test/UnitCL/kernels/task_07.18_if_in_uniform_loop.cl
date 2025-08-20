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

kernel void if_in_uniform_loop(global int *srcE, global int *srcO,
                               global int *dst) {
  size_t x = get_global_id(0);
  size_t n = get_global_size(0);
  int sum = 0;
  for (size_t i = 0; i < n; i++) {
    int val;
    if (x & 1)
      val = srcO[i] * 2;
    else
      val = srcE[i] * 3;
    sum += val;
  }
  dst[x] = sum;
}
