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
// Basic check for presence of DI entries for local variables, including
// kernel parameters.

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

// Assert subprogram contains a reference to present local variables
// CHECK: void @__mux_host_0({{.*}} !dbg [[DI_SUBPROGRAM:![0-9]+]]

// 'in' parameter IR location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_IN:![0-9]+]], metadata !DIExpression()

// 'out' parameter IR location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_OUT:![0-9]+]], metadata !DIExpression()

// 'gid' variable IR location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_GID:![0-9]+]], metadata !DIExpression()

// Assert we have DI types
// CHECK: [[DI_BASIC:![0-9]+]] = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
// CHECK: [[DI_DERIVED:![0-9]+]] = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: [[DI_BASIC:![0-9]+]], size: {{32|64}})

// Assert we can find kernel arguments
// CHECK: [[DI_IN]] = !DILocalVariable(name: "in", arg: 1,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 11, type: [[DI_DERIVED]]

// CHECK: [[DI_OUT]] = !DILocalVariable(name: "out", arg: 2,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 12, type: [[DI_DERIVED]]

// Assert we can find local variable g_id
// CHECK: [[DI_GID]] = !DILocalVariable(name: "g_id",
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 13, type: [[DI_BASIC]]
