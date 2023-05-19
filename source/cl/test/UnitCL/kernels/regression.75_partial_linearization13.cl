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

__kernel void partial_linearization13(__global int *out, int n) {
  size_t tid = get_global_id(0);
  size_t size = get_local_size(0);
  out[tid] = 0;
  if (tid + 1 < size) {
    out[tid] = n;
  } else if (tid + 1 == size) {
    size_t leftovers = 1 + (size & 1);
    switch (leftovers) {
      case 2:
        out[tid] = 2 * n + 1;
        // fall through
      case 1:
        out[tid] += 3 * n - 1;
        break;
    }
    switch (leftovers) {
      case 2:
        out[tid] /= n;
      // fall through
      case 1:
        out[tid]--;
        break;
    }
  }
}
