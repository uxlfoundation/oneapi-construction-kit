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

typedef struct {
  char a[3];
  short2 b;
  uchar3 offset1;
  ulong c;
  int offset2;
  float3 d;
} testStruct;

__constant testStruct bar = {{'a', 'b', 'c'}, {1, 2}, {'x', 'y', 'z'}, 42u, 42,
                             {0.9, 0.8, 0.7}};

kernel void constant_struct_alignment(__global ulong* mask,
                                      __global ulong* out) {
  out[0] = ((ulong)&bar.b) & mask[0];
  out[1] = ((ulong)&bar.c) & mask[1];
  out[2] = ((ulong)&bar.d) & mask[2];
}
