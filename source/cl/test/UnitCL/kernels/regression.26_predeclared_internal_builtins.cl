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

// REQUIRES: images nospirv

kernel void failed_function(global int *src, global int *dst,
                            read_only image2d_t img, sampler_t smplr) {
  size_t gid = get_global_id(0);
  // Force masked loads and stores
  if (gid) {
    // Uniform value
    int4 src_0 = vload4(0, src);
    dst[0] = src_0.s0;
    // We can't handle images
    dst[gid] = read_imagei(img, smplr, (int2)(2, 3)).x;
  }
}

kernel void predeclared_internal_builtins(global int *src, global int *dst,
                                          int i) {
  size_t gid = get_global_id(0);
  // Force masked loads and stores
  if (gid) {
    // Uniform value
    int4 src_0 = vload4(i, src);
    dst[i] = src_0.s0;
  }

  dst[gid] = src[gid];
}
