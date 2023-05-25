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
// Check that kernels which take C structs passed by value as parameters
// preserve debug info, and that there is sufficient DI for the struct.

// RUN: oclc -cl-options '-g -cl-opt-disable' %s -stage %stage -enqueue byval_struct > %t
// RUN: FileCheck < %t %s

typedef struct _my_struct
{
  int foo;
  int bar;
  int gee;
} my_struct;

void kernel byval_struct(global int *in,
                         my_struct my_str)
{
  const int idx = get_global_id(0);
  in[idx] = (idx * my_str.foo) + (my_str.bar * my_str.gee);
}

// Assert the DI entry is attached to the correct IR function
// CHECK: void @__mux_host_0({{.*}} !dbg [[DI_SUBPROGRAM:![0-9]+]] {

// Assert that the IR contains location declarations
// 'in' param location
// CHECK: [[IN_PARAM:%.+]] = alloca i32 addrspace(1)*
// CHECK: call void @llvm.dbg.declare({{.*}} [[IN_PARAM]], metadata [[DI_IN:![0-9]+]], metadata

// 'idx' variable location
// CHECK: call void @llvm.dbg.declare({{.*}} %idx{{.*}}, metadata [[DI_IDX:![0-9]+]], metadata

// 'my_str' struct param location
// CHECK: call void @llvm.dbg.declare({{.*}} %struct{{.*}}, metadata [[DI_STRUCT:![0-9]+]], metadata

// Assert we have all the necessary DI entries
// CHECK: [[DI_SUBPROGRAM]] = distinct !DISubprogram(name: "byval_struct",

// DI for my_struct type
// CHECK: [[DI_DERIVED_PTR:![0-9]+]] = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: [[DI_BASIC:![0-9]+]], size: {{32|64}})

// CHECK: [[DI_BASIC]] = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)

// CHECK: [[DI_DERIVED_TYPEDEF:![0-9]+]] = !DIDerivedType(tag: DW_TAG_typedef, name: "my_struct"
// CHECK-SAME: line: 15, baseType:  [[DI_COMPOSITE:![0-9]+]])

// CHECK: [[DI_COMPOSITE]] = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "_my_struct",
// CHECK-SAME: , line: 10, size: 96, elements: [[DI_ELEMS:![0-9]+]])

// CHECK: [[DI_ELEMS]] = !{[[DI_FOO:![0-9]+]], [[DI_BAR:![0-9]+]], [[DI_GEE:![0-9]+]]}

// CHECK: [[DI_FOO]] = !DIDerivedType(tag: DW_TAG_member, name: "foo",
// CHECK-SAME: line: 12, baseType: [[DI_BASIC]], size: 32)

// CHECK: [[DI_BAR]] = !DIDerivedType(tag: DW_TAG_member, name: "bar",
// CHECK-SAME: line: 13, baseType: [[DI_BASIC]], size: 32, offset: 32)

// CHECK: [[DI_GEE]] = !DIDerivedType(tag: DW_TAG_member, name: "gee",
// CHECK-SAME: line: 14, baseType: [[DI_BASIC]], size: 32, offset: 64)

// Local variable and parameter DI entries
// CHECK: [[DI_IN]] = !DILocalVariable(name: "in", arg: 1,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 17, type: [[DI_DERIVED_PTR]])

// CHECK: [[DI_IDX]] = !DILocalVariable(name: "idx",
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 20, type: [[DI_CONST:![0-9]+]])

// CHECK: [[DI_CONST]] = !DIDerivedType(tag: DW_TAG_const_type, baseType: [[DI_BASIC]])

// CHECK: [[DI_STRUCT]] = !DILocalVariable(name: "my_str", arg: 2,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 18, type: [[DI_DERIVED_TYPEDEF:![0-9]+]])
