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

__kernel void varying_load(__global int *out, int n, __global int *meta) {
  int id = get_global_id(0);
  int ret = 0;

  if (id <= 10) {
    int sum = n;
    if (*meta == 0) {
      int mul = n * id;
      int div = mul / n + id;
      int shl = div << 3;
      mul += shl;
      sum = mul << 3;
    }

    if (id % 2 == 0) {
      sum *= *meta + n;
      ret = sum;
    }
  }
  out[id] = ret;
}
