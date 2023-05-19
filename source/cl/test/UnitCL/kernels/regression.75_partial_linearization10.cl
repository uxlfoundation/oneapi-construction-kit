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

__kernel void partial_linearization10(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  int i = 0;

  while (1) {
    if (n > 0) {
      for (int i = 0; i < n * 2; i++) ret++;
      if (n <= 10) {
      goto f;
      }
    } else {
      for (int i = 0; i < n / 4; i++) ret++;
    }
    ret++;
    while (1) {
      if (n & 1) {
        if (n == 3) {
          goto j;
        }
      } else {
        if (ret + id >= n) {
          ret /= n * n + ret;
          goto o;
        }
      }
      if (i++ > 3) {
        ret += n * ret;
        goto n;
      }
      o:
        ret++;
    }
    j:
      if (n < 20) {
        ret += n * 2 + 20;
        goto p;
      } else {
        goto q;
      }
    n:
      ret *= 4;
    q:
      if (i > 5) {
        ret++;
        goto r;
      }
  }

r:
  for (int i = 0; i < n / 4; i++) ret++;
  goto s;

f:
  ret /= n;
  goto p;

p:
  for (int i = 0; i < n * 2; i++) ret++;

s:
  out[id] = ret;
}
