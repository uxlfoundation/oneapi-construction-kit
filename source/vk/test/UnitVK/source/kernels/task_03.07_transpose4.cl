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

__kernel void transpose4(__global int4 *in, __global int4 *out) {
  size_t tid = get_global_id(0);

  int4 sa = in[(tid * 4) + 0];
  int4 sb = in[(tid * 4) + 1];
  int4 sc = in[(tid * 4) + 2];
  int4 sd = in[(tid * 4) + 3];

  int4 da = (int4)(sa.x, sb.x, sc.x, sd.x);
  int4 db = (int4)(sa.y, sb.y, sc.y, sd.y);
  int4 dc = (int4)(sa.z, sb.z, sc.z, sd.z);
  int4 dd = (int4)(sa.w, sb.w, sc.w, sd.w);

  out[(tid * 4) + 0] = da;
  out[(tid * 4) + 1] = db;
  out[(tid * 4) + 2] = dc;
  out[(tid * 4) + 3] = dd;
}
