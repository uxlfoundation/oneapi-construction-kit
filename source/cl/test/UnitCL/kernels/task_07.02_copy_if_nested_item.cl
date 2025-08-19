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

__kernel void copy_if_nested_item(__global int *in, __global int *out1,
                                  __global int *out2) {
  size_t tid = get_global_id(0);
  size_t lid = get_local_id(0);
  if ((lid & 1) == 0) {
    int value = in[tid];
    if ((lid & 2) == 0) {
      out1[tid] = -value;
    }
    out2[tid] = value;
  }
}
