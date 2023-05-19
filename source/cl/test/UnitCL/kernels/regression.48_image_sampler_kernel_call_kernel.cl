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

void __kernel other_kernel(__global uint *out, __read_only image1d_t img,
                           sampler_t sampler1, sampler_t sampler2) {
  int gid = get_global_id(0);
  float coord = (float)gid;
  coord = coord / (get_global_size(0) / 2.0f) +
          0.05f;  // just to ensure it avoids being right on the edge of a pixel
  // First sampler passed will be repeat
  uint4 pxl1 = read_imageui(img, sampler1, coord);

  // Second sampler passed will be clamp
  uint4 pxl2 = read_imageui(img, sampler2, coord);

  out[gid * 2] = pxl1.x;
  out[gid * 2 + 1] = pxl2.x;
}

void __kernel image_sampler_kernel_call_kernel(__global uint *out,
                                               __read_only image1d_t img,
                                               sampler_t sampler1,
                                               sampler_t sampler2) {
  other_kernel(out, img, sampler1, sampler2);
}
