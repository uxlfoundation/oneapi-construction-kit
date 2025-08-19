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

__kernel void scatter_gather(int w, __global uint *in, __global uint *out) {
  uint width64 = (w + 63) & 0xFFFFFFC0;
  int global_id = get_global_id(0);
  uint target_width = width64;

  if (global_id < 4) {
    for (int x = 1; x <= w; x++) {
      out[x + (global_id * target_width)] =
          in[(x - 1) + (global_id * target_width)];
    }
  }
}
