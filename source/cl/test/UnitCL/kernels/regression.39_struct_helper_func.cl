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

// SPIR OPTIONS: "-w"
// SPIRV OPTIONS: "-w"

typedef struct {
  char offset;
  short3 m;
} testStruct;

testStruct returnHelper() {
  __private testStruct x = {'a', {1, 2, 3}};
  return x;
}

ulong passValHelper(testStruct x) { return (ulong) & (x.m); }

ulong passPtrHelper(testStruct *x) { return (ulong) & (x->m); }

kernel void struct_helper_func(__global ulong *mask, __global ulong *out) {
  testStruct myStruct = returnHelper();
  out[0] = passValHelper(myStruct) & mask[0];
  out[1] = passPtrHelper(&myStruct) & mask[1];
}
