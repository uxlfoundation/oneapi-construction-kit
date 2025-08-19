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

// REQUIRES: images

// This test has no real purpose besides preventing a regression when trying
// packetize a CFG that still has vector instructions in it, which triggers a
// call to `emitRemarkOptimizer` which was calling wrongly the ORE API.
void __kernel check_ore_call(__global uint *out, __write_only image2d_t img) {
  size_t gid = get_global_id(0);
  write_imagef(img, (int2)(gid, gid), (float4)(0.0f, 0.0f, 0.0f, 1.0f));
  out[gid] = 0;
}
