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

// SPIRV OPTIONS: -Wno-pointer-to-int-cast

struct struct_needs_32bit_align {
  char x;
  int y;
};
struct struct_needs_64bit_align {
  char x;
  long y;
};

struct struct_needs_1024byte_align {
  char c;
  short vec[3];
  int4 i4;
  long l;
} __attribute__((aligned(1024)));

__kernel void barrier_with_align(int mask1, int mask2, int mask3,
                                 __global int* output) {
  int global_id = get_global_id(0);

  struct struct_needs_32bit_align needs_32bit_align;
  struct struct_needs_1024byte_align needs_1024byte_align;
  struct struct_needs_64bit_align needs_64bit_align;

  needs_32bit_align.y = global_id + 12;
  needs_1024byte_align.l = 0xdeafbeef & global_id;
  needs_64bit_align.y = global_id + 54;

  barrier(CLK_LOCAL_MEM_FENCE);
  int num_out = 6;
  // use xor as testing for zeroes can lead to false positives
  output[global_id * num_out] = (((int)&needs_32bit_align.y) ^ mask1) & mask1;
  output[global_id * num_out + 1] =
      (((int)&needs_64bit_align.y) ^ mask2) & mask2;
  output[global_id * num_out + 2] =
      (((int)&needs_1024byte_align) ^ mask3) & mask3;
  output[global_id * num_out + 3] = needs_32bit_align.y;
  output[global_id * num_out + 4] = needs_64bit_align.y;
  output[global_id * num_out + 5] = needs_1024byte_align.l;
}
