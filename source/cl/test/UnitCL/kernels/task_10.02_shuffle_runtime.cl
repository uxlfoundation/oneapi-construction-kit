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

__kernel void shuffle_runtime(__global int4 *in1, __global int2 *in2,
                              uint4 mask1, uint2 mask2, __global int2 *out) {
  size_t gid = get_global_id(0);

  int4 splat = shuffle(in2[gid], mask1);
  out[gid] = shuffle2(splat, in1[gid], mask2);
}
