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

// REQUIRES: parameters

kernel void local_alignment(__global ulong *mask, __global ulong *out) {
  // Don't consecutively order the vector types so that
  // we test how optimial the padding is.
  __local TYPE mem0;
  __local TYPE8 mem8[SIZE * 3];
  __local TYPE mem1[SIZE];
  __local TYPE2 mem2[SIZE + 1][5];
  __local TYPE4 mem4[SIZE * 5];
  __local TYPE3 mem3[SIZE - 1];
  __local TYPE16 mem16[SIZE * 2];

  out[0] = (ulong)(&mem0) & mask[0];
  out[1] = (ulong)(&mem1[0]) & mask[1];
  out[2] = (ulong)(&mem2[0][0]) & mask[2];
  out[3] = (ulong)(&mem3[0]) & mask[3];
  out[4] = (ulong)(&mem4[0]) & mask[4];
  out[5] = (ulong)(&mem8[0]) & mask[5];
  out[6] = (ulong)(&mem16[0]) & mask[6];
}
