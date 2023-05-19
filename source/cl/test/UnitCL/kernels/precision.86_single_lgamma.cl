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

__kernel void single_lgamma(__global float* in, __global float *out) {
  size_t id = get_global_id(0);
  float2 in2 = (float2)(in[id * 2 + 0], in[id * 2 + 1]);
  float2 ret = lgamma(in2);
  out[id * 2 + 0] = ret[0];
  out[id * 2 + 1] = ret[1];
}
