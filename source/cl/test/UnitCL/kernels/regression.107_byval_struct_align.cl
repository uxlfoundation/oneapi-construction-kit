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


typedef struct _my_innermost_struct {
  char a;
} my_innermost_struct;

typedef struct _my_innermost_struct_holder {
  my_innermost_struct s;
} my_innermost_struct_holder;

typedef struct _my_innermost_struct_holder_holder {
  my_innermost_struct_holder s;
} my_innermost_struct_holder_holder;

typedef struct _my_struct_tuple {
  my_innermost_struct_holder_holder s;
  my_innermost_struct t;
} my_struct_tuple;

typedef struct _my_struct {
  my_innermost_struct_holder_holder s;
  my_struct_tuple t;
} my_struct;

__kernel void byval_struct_align(struct _my_struct s1, int v, __global int *outs1,
                                 struct _my_struct s2, int w, __global int *outs2) {

  const size_t idx = get_global_id(0);
  outs1[idx] = s1.t.t.a + v + w;
  outs2[idx] = s2.t.s.s.s.a + v + w;
}
