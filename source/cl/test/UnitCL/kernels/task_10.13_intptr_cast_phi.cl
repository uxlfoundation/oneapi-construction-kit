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

__kernel void intptr_cast_phi(__global char *in, __global int *out) {
  const size_t id = get_global_id(0);
  uintptr_t intin = (uintptr_t)in + (id << 4);
  for (int x = 0; x < 4; ++x) {
    out[(id << 2) + x] = *((__global int*)intin);
    intin += 4;
  }
}
