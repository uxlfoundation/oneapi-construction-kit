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

__kernel void partial_linearization17(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  int i = 0;

  while (1) {
    if (n > 10) {
      goto c;
    } else if (n > 5) {
      goto f;
    }
    if (id + i++ % 2 == 0) {
      break;
    }
  }

  for (int i = 0; i < n + 10; i++) ret++;
  goto m;

f:
  ret += n * 2;
  for (int i = 0; i < n * 2; i++) ret += i;
  goto m;

c:
  for (int i = 0; i < n + 5; i++) ret += 2;
  if (id % 2 == 0) {
    goto h;
  } else {
    goto m;
  }

m:
  ret <<= 2;
  goto o;

h:
  for (int i = 0; i < n * 2; i++) {
    if (n > 5) {
      goto l;
    }
  }
  ret += id << 3;
  goto p;

l:
  ret += id << 3;

o:
  for (int i = 0; i < n * 2; i++) ret += i;

p:
  out[id] = ret;
}
