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

// REQUIRES: double

#pragma OPENCL EXTENSION cl_khr_fp64 : enable
__kernel void fract_double(__global double* in, __global double* out,
                            __global double* out2) {
  size_t i = get_global_id(0);
  double f0 = in[i];
  double iout = NAN;
  f0 = fract(f0, &iout);
  out[i] = f0;
  out2[i] = iout;
}
