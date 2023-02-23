// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
