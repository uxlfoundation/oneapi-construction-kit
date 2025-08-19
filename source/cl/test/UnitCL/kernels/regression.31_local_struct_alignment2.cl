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

// REQUIRES: parameters

// Test array of vec types
typedef struct {
  uchar u;
  short3 s[2];
  float f;
  int2 c[4][4];
} __attribute__((aligned(ALIGN))) testStruct;

kernel void local_struct_alignment2(__global ulong *mask, __global ulong *out) {
  __local testStruct s[3];

  out[0] = (ulong)(&s[0]) & mask[0];
  out[1] = (ulong)(&s[1]) & mask[1];
  out[2] = (ulong)(&s[2]) & mask[2];
}
