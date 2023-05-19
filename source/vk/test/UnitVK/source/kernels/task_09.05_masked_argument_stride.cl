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

__kernel void masked_argument_stride(const __global int *input,
                                     __global int *output, int stride) {
  size_t gid = get_global_id(0);
  int index = get_global_id(0) * stride;
  if (gid != 0) {
    output[index] = input[index];
    output[index + 1] = 1;
    output[index + 2] = 1;
  } else {
    output[index] = 13;
    output[index + 1] = 13;
    output[index + 2] = 13;
  }
}
