// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Test Brief:
// Check that kernels which call user defined auxiliary functions preserve DI.
// Although these helper functions should be inlined, we need to ensure that
// this process doesn't hinder the DI of the kernel.

// RUN: %oclc -cl-options '-g -cl-opt-disable' %s -stage %stage -enqueue user_fn_identity > %t
// RUN: %filecheck < %t %s

int identity(int x)
{
  return x;
}

kernel void user_fn_identity(global int *in,
                             global int *out)
{
  size_t tid = get_global_id(0);
  out[tid] = identity(in[tid]);
}

kernel void unused_kernel(global int *in,
                          global int *out)
{
  out[get_global_id(0)] = in[get_global_id(0)];
}

// Assert the DI entry is attached to the correct function
// CHECK: void @__mux_host_0({{.*}} !dbg [[DI_SUBPROGRAM:![0-9]+]] {

// 'x' parameter of inlined helper function 'identity'
// CHECK: [[X_PARAM:%.+]] = alloca i32
// CHECK: call void @llvm.dbg.declare({{.*}} [[X_PARAM]], metadata [[DI_X:![0-9]+]], metadata

// 'in' param location
// CHECK: [[IN_PARAM:%.+]] = alloca i32 addrspace(1)*
// CHECK: call void @llvm.dbg.declare({{.*}} [[IN_PARAM]], metadata [[DI_IN:![0-9]+]], metadata

// 'out' param location
// CHECK: [[OUT_PARAM:%.+]] = alloca i32 addrspace(1)*
// CHECK: call void @llvm.dbg.declare({{.*}} [[OUT_PARAM]], metadata [[DI_OUT:![0-9]+]], metadata

// 'tid' local var location
// CHECK: call void @llvm.dbg.declare({{.*}} %tid{{.*}}, metadata [[DI_TID:![0-9]+]], metadata

// CHECK: [[DI_CU:![0-9]+]] = distinct !DICompileUnit
// CHECK: [[DI_FILE:![0-9]+]] = !DIFile

// DI entry for kernel function
// CHECK: {{![0-9]+}} = distinct !DISubprogram(name: "user_fn_identity", scope: [[DI_FILE]], file: [[DI_FILE]]
// CHECK-SAME: line: 16,
// CHECK-SAME: unit: [[DI_CU]]

// 'x' parameter from user identity function
// CHECK: [[DI_X]] = !DILocalVariable(name: "x", arg: 1, scope: [[DI_USER_FUNC:![0-9]+]]
// CHECK-SAME: line: 11

// DI entry for user function
// CHECK: [[DI_USER_FUNC]] = distinct !DISubprogram(name: "identity", scope: [[DI_FILE]], file: [[DI_FILE]]
// CHECK-SAME: line: 11
// CHECK-SAME: unit: [[DI_CU]]

// Kernel param DI entries
// CHECK: [[DI_IN]] = !DILocalVariable(name: "in", arg: 1,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}},  line: 16, type: !{{[0-9]+}}

// CHECK: [[DI_OUT]] = !DILocalVariable(name: "out", arg: 2,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}},  line: 17, type: !{{[0-9]+}}

// Local variable DI entries
// CHECK: [[DI_TID]] = !DILocalVariable(name: "tid",
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}},  line: 19, type: !{{[0-9]+}}
