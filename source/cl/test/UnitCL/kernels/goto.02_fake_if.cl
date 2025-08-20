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

__kernel void fake_if(__global int *dst) {
  size_t gid = get_global_id(0);

  bool jmp = (gid % 2) == 1;

  if (jmp == 0) goto left;
  if (jmp == 1) goto right;
  goto end; // Unreachable;

  left: {
    dst[gid] = 0;
    goto end;
  }

  right: {
    dst[gid] = 1;
    goto end;
  }

  end: {}
}
