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

// Removed inline from this function due to the clang SPIR generator dropping
// the function body when its present, this is a bug in the generator, this is
// a work around and retains the function body.
int WeirdPhi(const bool copy) {
  int result = 0;
  for (int k = 0; k < 2; ++k) {
    if (copy) {
      result = k;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
  }
  return result;
}

__kernel void barrier_inline_stray_phi(__global int* out) {
  out[get_global_id(0)] = WeirdPhi(true);
}
