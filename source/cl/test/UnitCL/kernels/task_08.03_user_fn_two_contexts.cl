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

int foo(int x, int y) { return x * (y - 1); }

kernel void user_fn_two_contexts(global int *out, global int *in,
                                 global int *in2, int alpha) {
  size_t tid = get_global_id(0);
  int src1 = in[tid];
  int src2 = in2[tid];
  int res1 = foo(src1, src2);   // varying, varying params
  int res2 = foo(alpha, src2);  // uniform, varying params
  out[tid] = res1 + res2;
}
