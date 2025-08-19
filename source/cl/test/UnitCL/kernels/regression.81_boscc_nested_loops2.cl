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

__kernel void boscc_nested_loops2(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  if (id < 16) {
    int mul = n * id;
    int div = mul / n + id;
    int shl = div << 3;
    int x = mul + div + shl;
    for (int i = 0; i < n; ++i) {
      if (id <= 8) {
        int j = 0;
        while (true) {
          ret++;
          int mul2 = mul * mul;
          int div2 = mul2 / n;
          int shl2 = div2 << 3;
          ret += shl2 + x;
          if (id + j++ >= 4) {
            break;
          }
        }
      }
    }
  }
  out[id] = ret;
}
