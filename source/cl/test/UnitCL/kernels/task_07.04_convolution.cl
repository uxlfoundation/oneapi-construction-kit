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
//
// DEFINITIONS: "-DFILTER_SIZE=1"

#if defined(KTS_DEFAULTS)
#define FILTER_SIZE 3
#endif

__kernel void convolution(__global float *src, __global float *dst) {
  int x = get_global_id(0);
  int width = get_global_size(0);
  if ((x >= FILTER_SIZE) && (x < (width - FILTER_SIZE))) {
    float sum = 0.0f;
    float totalWeight = 0.0f;
    for (int i = -FILTER_SIZE; i <= FILTER_SIZE; i++) {
      float weight = convert_float(x - i);
      totalWeight += weight;
      sum += weight * src[x + i];
    }
    sum /= totalWeight;
    dst[x] = sum;
  } else {
    dst[x] = 0.0f;
  }
}
