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

struct struct_needs_1024byte_align {
  char c;
  short vec[3];
  int4 i4;
  long l;
} __attribute__((aligned(1024)));

__kernel void attribute_aligned(int mask, __global int* output) {
  int global_id = get_global_id(0);

  struct struct_needs_1024byte_align needs_1024byte_align;

  needs_1024byte_align.l = 0xdeafbeef & global_id;

  int num_out = 2;
  // use xor as testing for zeroes can lead to false positives
  output[global_id * num_out] = (((int)&needs_1024byte_align) ^ mask) & mask;
  output[global_id * num_out + 1] = needs_1024byte_align.l;
}
