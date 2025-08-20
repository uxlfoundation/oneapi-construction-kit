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
//
// DEFINITIONS: "-DKTS_DEFAULTS=ON"

#if defined(KTS_DEFAULTS)
#define TRIPS 256
#endif

__kernel void saxpy_static_trip_uniform(__global float *x, __global float *y,
                                        __global float *out, const float a) {
  size_t tid = get_global_id(0);
  size_t lid = get_local_id(0);

  float sum = 0.0f;
  for (int i = 0; i < TRIPS; i++) {
    // SAXPY!
    int p = i + lid;
    sum += (a * x[p]) + y[p];
  }

  out[tid] = sum;
}
