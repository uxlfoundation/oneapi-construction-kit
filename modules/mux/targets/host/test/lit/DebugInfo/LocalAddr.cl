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
// Check DI is preserved in the presence of kernel
// variables allocating data in the __local address space.

// TODO(CA-1979): host: DebugInfo/LocalAddr.cl lit test failure on Arm
// in fact it also fails on "native" but the "native" feature was broken
// see comment on above ticket.
// XFAIL: *
// RUN: oclc -cl-options '-g -cl-opt-disable' %s -stage %stage -enqueue local_array > %t
// RUN: FileCheck < %t %s

kernel void local_array(global int *in, global int *out) {
  size_t tid = get_global_id(0);

  __local int data[1];
  __local int cache[3];
  data[0] = in[tid];

  if (get_local_id(0) == 0) {
    cache[0] = get_local_size(0);
    cache[1] = get_local_size(1);
    cache[2] = get_local_size(2);
  }

  out[tid] = data[0];
}

// Assert the DI entry is attached to the correct function
// CHECK: void @__mux_host_0({{.*}} !dbg [[DI_SUBPROGRAM:![0-9]+]] {

// Struct containing __local address space automatic variables
// CHECK: alloca %localVarTypes, align 4

// Debug intrinsics for each source variable contained in struct
// CHECK: call void @llvm.dbg.declare(metadata %localVarTypes* {{.*}}, metadata [[DI_VAR1:![0-9]+]], metadata !DIExpression(DW_OP_plus_uconst, 0)), !dbg [[DI_LOC1:![0-9]+]]
// CHECK: call void @llvm.dbg.declare(metadata %localVarTypes* {{.*}}, metadata [[DI_VAR2:![0-9]+]], metadata !DIExpression(DW_OP_plus_uconst, 4)), !dbg [[DI_LOC2:![0-9]+]]

// Assert we have the global variables in the CU entry
// CHECK: !DICompileUnit(
// CHECK-SAME: globals: [[DI_GLOBALS:![0-9]+]]

// List of global variables should now be empty
// CHECK: [[DI_GLOBALS]] = !{}

// Assert we have DI entry for kernel
// CHECK: [[DI_SUBPROGRAM]] = distinct !DISubprogram(name: "local_array",

// Assert kernel is associated with a file
// CHECK-SAME: file: [[DI_FILE:![0-9]+]]

// Assert kernel has a line number
// CHECK-SAME: line: 10,

// CHECK: [[DI_INT_TYPE:![0-9]+]] = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)

// Check debug info for 'data' __local address space variable
// CHECK: [[DI_VAR1]] = !DILocalVariable(name: "data", scope: [[DI_SUBPROGRAM]], file: [[DI_FILE]], line: 13, type: [[DI_DATA_TYPE:![0-9]+]])
// CHECK: [[DI_DATA_TYPE]] = !DICompositeType(tag: DW_TAG_array_type, baseType: [[DI_INT_TYPE]], size: 32, elements: [[DI_DATA_ELEMS:![0-9]+]])
// CHECK: [[DI_DATA_ELEMS]] = !{[[DI_SUBRANGE_1:![0-9]+]]}
// CHECK: [[DI_SUBRANGE_1]] = !DISubrange(count: 1)

// Expression pointing to offset in struct
// CHECK: [[DI_LOC1]] = !DILocation(line: 13, scope: [[DI_SUBPROGRAM]])

// Check debug info for 'cache' __local address space variable
// CHECK: [[DI_VAR2]] = !DILocalVariable(name: "cache", scope: [[DI_SUBPROGRAM]], file: [[DI_FILE]], line: 14, type: [[DI_CACHE_TYPE:![0-9]+]])
// CHECK: [[DI_CACHE_TYPE]] = !DICompositeType(tag: DW_TAG_array_type, baseType: [[DI_INT_TYPE]], size: 96, elements: [[DI_CACHE_ELEMS:![0-9]+]])
// CHECK: [[DI_CACHE_ELEMS]] = !{[[DI_SUBRANGE_3:![0-9]+]]}
// CHECK: [[DI_SUBRANGE_3]] = !DISubrange(count: 3)

// Expression pointing to offset in struct
// CHECK: [[DI_LOC2]] = !DILocation(line: 14, scope: [[DI_SUBPROGRAM]])
