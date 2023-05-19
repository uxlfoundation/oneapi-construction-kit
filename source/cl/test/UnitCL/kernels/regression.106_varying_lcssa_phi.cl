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

// This tests that the loop-computed value is handled correctly by the control
// flow conversion, namely that its value depends on the global ID despite
// having no direct dependence on it except through the control flow.
__kernel void varying_lcssa_phi(__global ushort *src, __global ushort *dst) {
  size_t x = get_global_id(0);
  unsigned short hash = 0;
  for (size_t i = 0; i < x; ++i) {
    // We explicitly cast 40499 to a uint here to avoid UB. If an
    // expression contains a ushort and ushort can be represented as int it
    // will be promoted to int in the expression. If the result of experession
    // then overflows int, you'll get UB.
    hash = hash * (uint)40499 + src[i];
  }

  if (hash & 1) {
    dst[x] = src[x];
  } else {
    dst[x] = hash;
  }
}
