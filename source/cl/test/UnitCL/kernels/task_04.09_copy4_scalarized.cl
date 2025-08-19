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

__kernel void copy4_scalarized(__global int *in, __global int *out) {
  size_t tid = get_global_id(0);
  size_t base = tid * 4;

  int x = in[base + 0];
  int y = in[base + 1];
  int z = in[base + 2];
  int w = in[base + 3];

  out[base + 0] = x;
  out[base + 1] = y;
  out[base + 2] = z;
  out[base + 3] = w;
}
