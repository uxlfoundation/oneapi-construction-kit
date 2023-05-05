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
// Tests non spec build option '-S <path>'. Option sets the debug info
// source location of the kernel to the specified file path. This file is
// created by the runtime if it does not already exist.

// RUN: %oclc -cl-options '-g -S %T%separatorgenerated.cl' %s -enqueue getsquare -stage %stage > %t
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

// Get CU file
// CHECK: !DICompileUnit(language: DW_LANG_C99, file: [[DI_FILE:![0-9]+]]

// Directory path will change depending on Windows/Linux.
// '\5C' is expected file path delimiter for windows, used in the LLVM tests.
// CHECK: [[DI_FILE]] = !DIFile(filename: "generated.cl", directory: "{{.*(\\5C|\/)}}lit{{(\\5C|\/)}}DebugInfo{{(\\5C|\/)}}Output")

// Assert kernel DI entry is linked to the file DI entry
// CHECK: DISubprogram(
// CHECK-SAME: file: [[DI_FILE]]
