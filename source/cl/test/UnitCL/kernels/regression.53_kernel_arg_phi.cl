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
// DEFINITIONS: "-DSIZE=45";"-DLOOPS=5"

__kernel void kernel_arg_phi(__global uchar* dst_ptr, int dst_step) {
  const int x = get_group_id(0);
  const int y = get_global_id(1);

  const int block_size = SIZE / LOOPS;

  __local int2 smem[SIZE];

  for (int i = 0; i < LOOPS; i++) {
    smem[y + i * block_size] = (int2)('A', 'B');
  }

  barrier(CLK_LOCAL_MEM_FENCE);

  __global uchar* dst = dst_ptr + mad24(x, (int)sizeof(int2), y * dst_step);
  for (int i = 0; i < LOOPS; i++, dst += block_size * dst_step) {
    vstore2(smem[y + i * block_size], 0, (__global int*)dst);
  }
}
