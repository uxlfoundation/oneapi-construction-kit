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
// Basic check for presence of DI entry describing a global static variable.

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

// LLVM global with dbg attachment
// CHECK: @count = internal addrspace(2) constant i32 1, align 4, !dbg [[DI_GLOBAL_VAR_EXPR:![0-9]+]]

// Assert we can find global __const variable 'count'
// CHECK: [[DI_GLOBAL_VAR_EXPR]] = !DIGlobalVariableExpression(var: [[DI_GLOBAL_VAR:![0-9]+]],
// CHECK-SAME: expr: !DIExpression()

// CHECK: [[DI_GLOBAL_VAR]] = distinct !DIGlobalVariable(name: "count",
// CHECK-SAME: scope: [[DI_CU:![0-9]+]], file: !{{[0-9]+}}, line: 9, type: [[DI_BASIC:![0-9]+]]

// Assert CU contains a reference to present global variables
// CHECK: [[DI_CU]] = distinct !DICompileUnit(
// CHECK-SAME: globals: [[DI_GLOBALS:![0-9]+]]
// CHECK: [[DI_GLOBALS]] = !{[[DI_GLOBAL_VAR_EXPR]]}

// Assert type of count is int
// CHECK: [[DI_BASIC]] = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
