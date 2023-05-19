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

// The same as "barriers_with_alias" except manually writing the
// same optimizations that clang-level -O3 does.

struct mystruct {
  int x[3];
};

__kernel void barriers_with_alias_2(__global int* output) {
  int global_id = get_global_id(0);

  struct mystruct foo;

  foo.x[1] = 22;

  int* fooptr = (global_id & 1) ? &foo.x[1]  // -> 22
                                : &foo.x[0]; // -> 20

  barrier(CLK_LOCAL_MEM_FENCE);

  foo.x[0] = 1;  // *foo2.ptr should now become 1

  int total = *fooptr /* 1 or 22 */ + global_id;

  fooptr++;            // moves onto y or z
  barrier(CLK_LOCAL_MEM_FENCE);
  foo.x[1] = 12;  // update original foo
  foo.x[2] = 14;
  total = total + *fooptr;  // add 12 or 14.

  output[global_id] = total;
}
