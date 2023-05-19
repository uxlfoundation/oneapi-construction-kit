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

__kernel void boscc_nested_loops1(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  if (id % 2 == 0) {
    const bool cmp = n == 5;
    int mul = n * id;
    int div = mul / n + id;
    int shl = div << 3;
    int x = shl + mul + div;
    for (int i = 0; i < n; ++i) {
      if (cmp) {
        ret += x;
      }
      if (n % 2 != 0) {
        if (n > 3) {
          for (int j = 0; j < n; ++j) {
            ret++;
            if (id == 0) {
              int mul2 = mul * mul;
              int div2 = mul2 / n;
              int shl2 = div2 << 3;
              ret += shl2;
            }
            for (int k = 0; k < n; ++k) {
              ret += x;
              if (id == 4) {
                int mul2 = mul * mul;
                int div2 = mul2 / n;
                int shl2 = div2 << 3;
                ret += shl2;
              }
            }
          }
        }
      }
    }
  }
  out[id] = ret;
}
