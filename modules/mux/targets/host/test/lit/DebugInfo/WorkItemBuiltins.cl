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
// Check that kernels which call work item builtin functions
// preserve debug info. These calls will be inlined so are
// hard to check the DI for. In the future however we may want
// to call them in debugger expression evaluation.

// RUN: oclc -cl-options '-g -cl-opt-disable' %s -stage %stage -enqueue work_item_builtins > %t
// RUN: FileCheck < %t %s

kernel void work_item_builtins(global int *out)
{
  size_t g_id = get_global_id(0);
  size_t g_size = get_global_size(0);
  size_t l_id = get_local_id(0);
  size_t l_size = get_local_size(0);

  uint w_dim = get_work_dim();
  size_t n_groups = get_num_groups(0);
  size_t group_id = get_group_id(0);

  out[g_id] = g_size + l_id + l_size + w_dim + n_groups + group_id;
}

// Assert the DI entry is attached to the correct function
// CHECK: void @__mux_host_0({{.*}} !dbg [[DI_SUBPROGRAM:![0-9]+]] {

// 'out' param location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_OUT:![0-9]+]], metadata !DIExpression()

// 'g_id' local var location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_GID:![0-9]+]], metadata !DIExpression()

// 'g_size' local var location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_GSIZE:![0-9]+]], metadata !DIExpression()

// 'l_id' local var location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_LID:![0-9]+]], metadata !DIExpression()

// 'l_size' local var location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_LSIZE:![0-9]+]], metadata !DIExpression()

// 'w_dim' local var location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_WDIM:![0-9]+]], metadata !DIExpression()

// 'n_groups' local var location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_GROUPS:![0-9]+]], metadata !DIExpression()

// 'group_id' local var location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_GROUP:![0-9]+]], metadata !DIExpression()

// DI entry for kernel function
// CHECK: [[DI_SUBPROGRAM]] = distinct !DISubprogram(name: "work_item_builtins",
// CHECK-SAME: line: 12,

// Kernel param DI entry
// CHECK: [[DI_OUT]] = !DILocalVariable(name: "out", arg: 1,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 12, type: !{{[0-9]+}}

// Local variable DI entries
// CHECK: [[DI_GID]] = !DILocalVariable(name: "g_id
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 14, type: !{{[0-9]+}}

// Local variable DI entries
// CHECK: [[DI_GSIZE]] = !DILocalVariable(name: "g_size"
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 15, type: !{{[0-9]+}}

// CHECK: [[DI_LID]] = !DILocalVariable(name: "l_id"
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 16, type: !{{[0-9]+}}

// CHECK: [[DI_LSIZE]] = !DILocalVariable(name: "l_size"
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 17, type: !{{[0-9]+}}

// CHECK: [[DI_WDIM]] = !DILocalVariable(name: "w_dim"
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 19, type: !{{[0-9]+}}

// CHECK: [[DI_GROUPS]] = !DILocalVariable(name: "n_groups"
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 20, type: !{{[0-9]+}}

// CHECK: [[DI_GROUP]] = !DILocalVariable(name: "group_id"
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 21, type: !{{[0-9]+}}
