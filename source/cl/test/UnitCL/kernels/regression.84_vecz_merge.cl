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

__kernel void vecz_merge(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;

  while (1) {
    if (n > 0 && n < 5) {
      goto f;
    }
    while (1) {
      if (n <= 2) {
        ret = 5;
        goto f;
      } else {
        if (ret + id >= n) {
          ret = id;
          goto d;
        }
      }
      if (n & 1) {
        ret = 1;
        goto f;
      }

d:
      if (n > 3) {
        ret = n;
        goto e;
      }
    }

e:
    if (n & 1) {
      ret = n + 2;
      goto f;
    }
  }

f:
  out[id] = ret;
}
