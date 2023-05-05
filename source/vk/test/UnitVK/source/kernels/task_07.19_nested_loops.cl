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

kernel void nested_loops(global int *img, global int *stridesX, global int *dst,
                         int width, int height) {
  size_t tid = get_global_id(0);
  size_t strideX = stridesX[tid];
  int sum = 0;
  for (size_t j = 0; j < height; j++) {
    for (size_t i = 0; i < width; i += strideX) {
      sum += img[(j * width) + i];
    }
  }
  dst[tid] = sum;
}
