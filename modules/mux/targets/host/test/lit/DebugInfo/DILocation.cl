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

// Test Brief:
// Basic check for presence of DI entries describing source line locations
// of IR instructions.

// RUN: %oclc -cl-options '-g -cl-opt-disable' %s -stage %stage -enqueue getsquare > %t
// RUN: %filecheck < %t %s

static __constant int count = 1;
kernel void getsquare(global int *in,
                      global int *out) {
  int g_id = get_global_id(0);
  if (g_id < count)
  {
    out[g_id] = in[g_id] * in[g_id];
  }
}

// Assert we have DI entries for each of the source lines
// CHECK: {{[0-9]+}} = distinct !DIGlobalVariable(name: "count",
// CHECK-SAME: line: 10
// CHECK: {{[0-9]+}} = !DILocation(line: 11
// CHECK: {{[0-9]+}} = !DILocation(line: 12
// CHECK: {{[0-9]+}} = !DILocation(line: 13
// CHECK: {{[0-9]+}} = !DILocation(line: 14
// CHECK: {{[0-9]+}} = !DILocation(line: 16
// CHECK: {{[0-9]+}} = !DILocation(line: 17
