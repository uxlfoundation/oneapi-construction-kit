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
  int x;
};

struct mystruct2 {
  int* ptr;
};

__kernel void barriers_with_alias(__global int* output) {
  int global_id = get_global_id(0);

  struct mystruct foo;

  foo.x = 20;

  struct mystruct2 foo2;
  if (global_id & 1) {
    foo.x += 2;
    foo2.ptr = &foo.x;  // -> 22
  } else {
    foo2.ptr = &foo.x;  // -> 20
  }

  barrier(CLK_LOCAL_MEM_FENCE);

  output[global_id] = **&foo2.ptr + global_id;
}
