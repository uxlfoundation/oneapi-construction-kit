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

typedef struct _my_struct {
  int foo;
  int bar;
  int gee;
} my_struct;

__kernel void byval_struct_align(__global int *out, ulong v1, __global int *in2, ulong v2,
   int v3, struct _my_struct s1, struct _my_struct s2, struct _my_struct s3) {

  const int idx = get_global_id(0);
  int r1 = (s1.foo - s1.bar) * s1.gee; // (2 - 1) * 2 = 2
  int r2 = (s2.foo - s2.bar) * s2.gee; // (4 - 3) * 5 = 5
  int r3 = (s3.foo - s3.bar) * s3.gee; // (6 - 9) * 7 = -21
  out[idx] = (idx * r1) + (r2 * 10 - r3); // idx * 2 + (5 * 10 - (-21))
}
