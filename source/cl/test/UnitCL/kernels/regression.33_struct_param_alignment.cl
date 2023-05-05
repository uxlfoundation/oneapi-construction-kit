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

typedef struct {
  char a;
  short3 b;
  uchar2 offset1;
  int* c;
  short offset2;
  float d[3];
  char offset3;
  long4 e;
} UserStruct;

kernel void struct_param_alignment(__global UserStruct* my_struct,
                                   __global ulong* mask, __global ulong* out) {
  out[0] = ((ulong)&my_struct[0].b) & mask[0];
  out[1] = ((ulong)&my_struct[1].c) & mask[1];
  out[2] = ((ulong)&my_struct[2].d[0]) & mask[2];
  out[3] = ((ulong)&my_struct[3].d[1]) & mask[3];
  out[4] = ((ulong)&my_struct[4].d[2]) & mask[4];
  out[5] = ((ulong)&my_struct[5].e) & mask[5];
}
