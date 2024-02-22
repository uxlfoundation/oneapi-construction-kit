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

// RUN: oclc -execute -enqueue halfMul -arg in,1.0,2.0,3.0,4.0 \
// RUN:   -print out,4 -global 4,1,1 -local 4,1,1 %s | FileCheck %s

__kernel void halfMul(global half *in, global half *out) {
  size_t gid = get_global_id(0);
  float x = vload_half(gid, in) * 2.0;
  vstore_half(x, gid, out);
}

// CHECK: out,2.000000,4.000000,6.000000,8.000000
