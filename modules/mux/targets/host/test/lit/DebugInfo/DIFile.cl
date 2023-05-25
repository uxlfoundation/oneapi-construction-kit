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
// Basic check for presence of DI entry describing the source file the kernel
// originated from. Since kernels are created from a C string passed into an
// API call the runtime has to create the source location. For the moment this
// is filename 'kernel.opencl', with the directory oclc is being run from.

// RUN: oclc -cl-options '-g -cl-opt-disable' %s -enqueue getsquare -stage %stage > %t
// RUN: FileCheck < %t %s

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
// CHECK: [[DI_FILE]] = !DIFile(filename: "kernel.opencl", directory: "{{.*}}DebugInfo")

// Assert kernel DI entry is linked to the file DI entry
// CHECK: DISubprogram(
// CHECK-SAME: file: [[DI_FILE:![0-9]+]]
