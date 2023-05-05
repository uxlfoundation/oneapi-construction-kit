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

__kernel void masked_interleaved_group(__global char *out, __global char *in) {
  int x = get_global_id(0) * 2;
  out[x] = 0;
  out[x + 1] = 0;

  // the barrier is to ensure the non-written values are initialised to zeroes,
  // but while ensuring we retain the masked stores in the second part.
  barrier(CLK_GLOBAL_MEM_FENCE);

  __global char *base_in = in + x;
  char v1 = base_in[0];
  char v2 = base_in[1];

  __global char *base_out = out + x;
  if (v1 + v2 < 0) {
    base_out[1] = v1;
  } else {
    base_out[0] = v2;
  }
}
