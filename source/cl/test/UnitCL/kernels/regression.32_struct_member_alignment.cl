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

struct node {
  struct node* ptr;
  struct node*** triple_ptr;
  short offset1;
  int16 x;
};

typedef struct {
  uchar2 offset2;
  struct node* linked_list;
} nestedStruct;

typedef struct {
  char offset3;
  TYPE m;
  nestedStruct struct_array[3][2];
} testStruct;

kernel void struct_member_alignment(__global ulong* mask, __global ulong* out) {
  __private testStruct s1[2];
  __private nestedStruct s2;
  __private struct node s3;

  out[0] = ((ulong)&s1[0].m) & mask[0];
  out[1] = ((ulong)&s2.linked_list) & mask[1];
  out[2] = ((ulong)&s3.x) & mask[2];
}
