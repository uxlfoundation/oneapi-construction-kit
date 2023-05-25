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
// Check that kernels with large numbers of arguments, of varying types,
// preserve DI for all the parameters.

// RUN: oclc -cl-options '-g -cl-opt-disable' %s -stage %stage -enqueue test_args > %t
// RUN: FileCheck < %t %s

kernel void test_args(global int *in1,
                      global float2 *in2,
                      const int in3,
                      local float *in4,
                      constant int *in5,
                      global int *in6,
                      global int2 *in7,
                      global int3 *in8,
                      global int4 *in9,
                      global int *out)
{
  out[get_global_id(0)] = in3;
}

// Assert the DI entry is attached to the correct IR function
// CHECK: void @__mux_host_0({{.*}} !dbg [[DI_SUBPROGRAM:![0-9]+]] {

// 'in1' param location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_IN1:![0-9]+]], metadata !DIExpression()

// 'in2' param location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_IN2:![0-9]+]], metadata !DIExpression()

// 'in3' param location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_IN3:![0-9]+]], metadata !DIExpression()

// 'in4' param location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_IN4:![0-9]+]], metadata !DIExpression()

// 'in5' param location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_IN5:![0-9]+]], metadata !DIExpression()

// 'in6' param location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_IN6:![0-9]+]], metadata !DIExpression()

// 'in7' param location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_IN7:![0-9]+]], metadata !DIExpression()

// 'in8' param location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_IN8:![0-9]+]], metadata !DIExpression()

// 'in9' param location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_IN9:![0-9]+]], metadata !DIExpression()

// 'out' param location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_OUT:![0-9]+]], metadata !DIExpression()

// Assert we have all the necessary DI entries
// CHECK: [[DI_SUBPROGRAM]] = distinct !DISubprogram(name: "test_args",

// Assert kernel is associated with a file
// CHECK-SAME: file: !{{[0-9]+}}

// Assert kernel has a line number
// CHECK-SAME: line: 10

// Parameter DI entries
// CHECK: [[DI_IN1]] = !DILocalVariable(name: "in1", arg: 1,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 10, type: !{{[0-9]+}}

// CHECK: [[DI_IN2]] = !DILocalVariable(name: "in2", arg: 2,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 11, type: !{{[0-9]+}}

// CHECK: [[DI_IN3]] = !DILocalVariable(name: "in3", arg: 3,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 12, type: !{{[0-9]+}}

// CHECK: [[DI_IN4]] = !DILocalVariable(name: "in4", arg: 4,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 13, type: !{{[0-9]+}}

// CHECK: [[DI_IN5]] = !DILocalVariable(name: "in5", arg: 5,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 14, type: !{{[0-9]+}}

// CHECK: [[DI_IN6]] = !DILocalVariable(name: "in6", arg: 6,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 15, type: !{{[0-9]+}}

// CHECK: [[DI_IN7]] = !DILocalVariable(name: "in7", arg: 7,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 16, type: !{{[0-9]+}}

// CHECK: [[DI_IN8]] = !DILocalVariable(name: "in8", arg: 8,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 17, type: !{{[0-9]+}}

// CHECK: [[DI_IN9]] = !DILocalVariable(name: "in9", arg: 9,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 18, type: !{{[0-9]+}}

// CHECK: [[DI_OUT]] = !DILocalVariable(name: "out", arg: 10,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 19, type: !{{[0-9]+}}
