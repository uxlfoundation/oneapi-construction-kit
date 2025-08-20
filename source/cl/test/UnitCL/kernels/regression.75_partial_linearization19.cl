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

__kernel void partial_linearization19(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  int i = 0;

  while (1) {
    if (n > 5) {
      if (n == 6) {
        goto d;
      } else {
        goto e;
      }
    }
    if (++i + id > 3) {
      break;
    }
  }

  if (n == 3) {
    goto h;
  } else {
    goto i;
  }

d:
  for (int i = 0; i < n + 5; i++) ret += 2;
  goto i;

e:
  for (int i = 1; i < n * 2; i++) ret += i;
  goto h;

i:
  for (int i = 0; i < n + 5; i++) ret++;
  goto j;

h:
  for (int i = 0; i < n; i++) ret++;
  goto j;

j:
  out[id] = ret;
}
