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

struct S {
  int a;
  int b;
  int c;
  int d;
  float e;
  int f;
};

__kernel void memcpy_optimization(__global int4 *in, __global int4 *out) {
  size_t gid = get_global_id(0);
  int4 tmp = in[gid];
  struct S in_s = {tmp.s0, tmp.s1, 42, tmp.s2, 25.81, tmp.s3};
  struct S out_s = in_s;
  out[gid] = (int4)(out_s.a, out_s.b, out_s.d, out_s.f);
}
