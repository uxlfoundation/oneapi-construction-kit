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

kernel void masked_loop_varying(global int *src, global int *dst, int n) {
  int x = get_global_id(0);
  int size = get_global_size(0);
  if ((x + n) <= size) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
      sum += src[x + i];
    }
    dst[x] = sum;
  }
}
