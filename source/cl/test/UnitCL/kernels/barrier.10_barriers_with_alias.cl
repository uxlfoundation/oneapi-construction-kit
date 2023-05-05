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

struct mystruct {
  int x[3];
};

struct mystruct2 {
  int* ptr;
};

struct mystruct3 {
  int** ptr;
};

__kernel void barriers_with_alias(__global int* output) {
  int global_id = get_global_id(0);

  struct mystruct foo;

  foo.x[0] = 20;
  foo.x[1] = 22;
  foo.x[2] = 24;

  struct mystruct2 foo2;
  if (global_id & 1) {
    foo2.ptr = &foo.x[1];  // -> 22
  } else {
    foo2.ptr = &foo.x[0];  // -> 20
  }

  barrier(CLK_LOCAL_MEM_FENCE);

  foo.x[0] = 1;  // *foo2.ptr should now become 1

  int total = *foo2.ptr /* 1 or 22 */ + global_id;

  struct mystruct3 foo3;
  foo2.ptr++;            // moves onto y or z
  foo3.ptr = &foo2.ptr;  // change foo3's ptr to be foo2's ptr
  barrier(CLK_LOCAL_MEM_FENCE);
  foo.x[1] = 12;  // update original foo
  foo.x[2] = 14;
  total = total + **foo3.ptr;  // add 12 or 14.

  output[global_id] = total;
}
