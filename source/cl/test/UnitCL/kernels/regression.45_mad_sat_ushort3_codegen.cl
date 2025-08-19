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

__kernel void mad_sat_ushort3_codegen(__global ushort *srcA,
                                      __global ushort *srcB,
                                      __global ushort *srcC,
                                      __global ushort *dst) {
  int gid = get_global_id(0);
  ushort3 sA = vload3(gid, srcA);
  ushort3 sB = vload3(gid, srcB);
  ushort3 sC = vload3(gid, srcC);
  ushort3 result = mad_sat(sA, sB, sC);
  vstore3(result, gid, dst);
}
