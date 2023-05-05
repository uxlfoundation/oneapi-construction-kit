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

__kernel void interleaved_load_5(__global int *out, __global int *in, int stride) {
  int x = get_global_id(0);
  int y = get_global_id(1);

  int v1 = in[(x + y * stride) * 2];
  int v2 = in[(x + y * stride) * 2 + 1];
  int v3 = in[(x + y * stride) * 2 + 2];
  int v4 = in[(x + y * stride) * 2 + 3];

  out[x + y * stride] = v1 + v2 + v3 + v4;
}
