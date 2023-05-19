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
// Basic check for presence of DI entry describing program compile unit.

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

// Assert we have a compile unit
// CHECK: [[DI_CU:![0-9]+]] = distinct !DICompileUnit(language: DW_LANG_C99

// Assert CU has a source file
// CHECK-SAME: file: !{{[0-9]+}}

// Assert CU contains globals
// CHECK-SAME: globals: !{{[0-9]+}}

// Make sure function points to correct CU
// CHECK: !DISubprogram(name: "getsquare"
// CHECK-SAME:, unit: [[DI_CU]],
