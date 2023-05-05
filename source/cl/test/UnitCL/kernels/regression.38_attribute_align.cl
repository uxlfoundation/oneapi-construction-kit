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

// REQUIRES: parameters

typedef struct {
  char pad;
  short __attribute__((aligned(ALIGN1))) x;
} __attribute__((aligned(ALIGN2))) testStruct;

__constant testStruct constantStruct = {'a', 42};

kernel void attribute_align(__global ulong *mask, __global ulong *out) {
  __private testStruct privateStruct;
  __local testStruct localStruct;

  out[0] = ((ulong)&privateStruct.x) & mask[0];
  out[1] = ((ulong)&localStruct.x) & mask[1];
  out[2] = ((ulong)&constantStruct.x) & mask[2];

  out[3] = ((ulong)&privateStruct) & mask[3];
  out[4] = ((ulong)&localStruct) & mask[4];
  out[5] = ((ulong)&constantStruct) & mask[5];
}
