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

__kernel void partial_linearization5(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;

  if (id % 2 == 0) {
    if (id == 4) {
      goto g;
    } else {
      goto d;
    }
  } else {
    if (n % 2 == 0) {
      goto d;
    } else {
      goto e;
    }
  }

d:
  for (int i = 0; i < n; i++) { ret += i - 2; }
  goto f;

e:
  for (int i = 0; i < n + 5; i++) { ret += i + 5; }

f:
  ret *= ret % n;
  ret *= ret + 4;

g:
  out[id] = ret;
}
