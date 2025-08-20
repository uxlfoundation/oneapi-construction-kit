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

__kernel void extract_vec3(__global int3 *src,
                           __global int *dstX,
                           __global int *dstY,
                           __global int *dstZ) {
  size_t tid = get_global_id(0);

  // For whatever reason, the `.xyz` prevents Clang from converting the load
  // from a load <3 x i32> to a load <4 x i32>
  int3 v = src[tid].xyz;
  dstX[tid] = v.x;
  dstY[tid] = v.y;
  dstZ[tid] = v.z;
}
