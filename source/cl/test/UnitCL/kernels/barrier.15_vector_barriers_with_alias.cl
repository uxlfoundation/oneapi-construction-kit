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

struct mystruct {
  int x[2];
  int* y;
};

__kernel void vector_barriers_with_alias(__global int* output) {
  int global_id = get_global_id(0);

  struct mystruct foo[4];

  foo[0].x[0] = 20;
  foo[0].x[1] = 22;

  foo[1].x[0] = 20;
  foo[1].x[1] = 22;

  foo[2].x[0] = 20;
  foo[2].x[1] = 22;

  foo[3].x[0] = 20;
  foo[3].x[1] = 22;

  if ((global_id * 4 + 0) & 1) {
    foo[0].y = &foo[0].x[1];  // -> 22
  } else {
    foo[0].y = &foo[0].x[0];  // -> 20
  }

  if ((global_id * 4 + 1) & 1) {
    foo[1].y = &foo[1].x[1];  // -> 22
  } else {
    foo[1].y = &foo[1].x[0];  // -> 20
  }

  if ((global_id * 4 + 2) & 1) {
    foo[2].y = &foo[2].x[1];  // -> 22
  } else {
    foo[2].y = &foo[2].x[0];  // -> 20
  }

  if ((global_id * 4 + 3) & 1) {
    foo[3].y = &foo[3].x[1];  // -> 22
  } else {
    foo[3].y = &foo[3].x[0];  // -> 20
  }

  barrier(CLK_LOCAL_MEM_FENCE);

  foo[0].x[0] = 1;  // *foo.y should now become 1
  foo[1].x[0] = 1;  // *foo.y should now become 1
  foo[2].x[0] = 1;  // *foo.y should now become 1
  foo[3].x[0] = 1;  // *foo.y should now become 1

  int total0 = *foo[0].y /* 1 or 22 */ + global_id * 4 + 0;
  int total1 = *foo[1].y /* 1 or 22 */ + global_id * 4 + 1;
  int total2 = *foo[2].y /* 1 or 22 */ + global_id * 4 + 2;
  int total3 = *foo[3].y /* 1 or 22 */ + global_id * 4 + 3;

  output[global_id * 4 + 0] = total0;
  output[global_id * 4 + 1] = total1;
  output[global_id * 4 + 2] = total2;
  output[global_id * 4 + 3] = total3;
}
