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

kernel void insertelement_runtime_index(global int4* in, global int4* out,
                                        global int* index) {
  size_t gid = get_global_id(0);

  out[gid] = in[gid];

  // This is not actually legal OpenCL C, the only specified way to access
  // elements of a vector is via either .xyzw, or .s0123 style syntax.
  // However, Clang accepts this and the test specifically depends on the index
  // being a runtime value (the other syntaxes are implicitly compile time
  // indices).
  out[gid][index[gid]] = 42;
}
