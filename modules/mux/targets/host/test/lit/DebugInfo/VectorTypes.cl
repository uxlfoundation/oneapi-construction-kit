// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Test Brief:
// Check that using opencl vector types preserves debug info, and that these
// types are faithfully represented in the DI.

// RUN: %oclc -cl-options '-g -cl-opt-disable' %s -stage %stage -enqueue add3 > %t
// RUN: %filecheck < %t %s

kernel void add3(global int3 *in1,
                 global int3 *in2,
                 global int3 *out)
{
  size_t tid = get_global_id(0);

  int3 a = vload3(0, (global int *)&in1[tid]);
  int3 b = vload3(0, (global int *)&in2[tid]);
  int3 c = a + b;

  vstore3(c, 0, (global int *)&out[tid]);
}

// Assert the DI entry is attached to the correct IR function
// CHECK: void @__mux_host_0({{.*}} !dbg [[DI_SUBPROGRAM:![0-9]+]] {

// 'in1' param location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_IN1:![0-9]+]], metadata !DIExpression()

// 'in2' param location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_IN2:![0-9]+]], metadata !DIExpression()

// 'out' param location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_OUT:![0-9]+]], metadata !DIExpression()

// 'tid' variable location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_TID:![0-9]+]], metadata !DIExpression()

// 'a' variable location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_A:![0-9]+]], metadata !DIExpression()

// 'b' variable location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_B:![0-9]+]], metadata !DIExpression()

// 'c' variable location
// CHECK: call void @llvm.dbg.declare({{.*}}, metadata [[DI_C:![0-9]+]], metadata !DIExpression()

// CHECK: [[DI_BASIC:![0-9]+]] = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)

// Assert we have the DI for kernel function
// CHECK: [[DI_SUBPROGRAM]] = distinct !DISubprogram(name: "add3",

// Assert kernel is associated with a file
// CHECK-SAME: file: !{{[0-9]+}}

// Assert kernel has a line number
// CHECK-SAME: line: 10

// Assert DI describing vector types is present
// CHECK: [[DI_POINTER:![0-9]+]] = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: [[DI_TYPEDEF:![0-9]+]], size: {{32|64}})
// CHECK: [[DI_TYPEDEF]] = !DIDerivedType(tag: DW_TAG_typedef, name: "int3",
// CHECK-SAME: line: {{[1-9][0-9]+}}, baseType: [[DI_COMPOSITE:![0-9]+]]

// CHECK: [[DI_COMPOSITE]] = !DICompositeType(tag: DW_TAG_array_type, baseType: [[DI_BASIC]], size: 128, flags: DIFlagVector, elements: [[DI_ELEM:![0-9]+]])

// CHECK: [[DI_ELEM]] = !{[[DI_SUBRANGE:![0-9]+]]}

// CHECK: [[DI_SUBRANGE]] = !DISubrange(count: 3)

// Parameter DI entries
// CHECK: [[DI_IN1]] = !DILocalVariable(name: "in1", arg: 1,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 10, type: [[DI_POINTER]])

// CHECK: [[DI_IN2]] = !DILocalVariable(name: "in2", arg: 2,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 11, type: [[DI_POINTER]])

// CHECK: [[DI_OUT]] = !DILocalVariable(name: "out", arg: 3,
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 12, type: [[DI_POINTER]])

// Local variable DI entries
// CHECK: [[DI_TID]] = !DILocalVariable(name: "tid",
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 14, type: !{{[0-9]+}}

// CHECK: [[DI_A]] = !DILocalVariable(name: "a",
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 16, type: [[DI_TYPEDEF]])

// CHECK: [[DI_B]] = !DILocalVariable(name: "b",
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 17, type: [[DI_TYPEDEF]])

// CHECK: [[DI_C]] = !DILocalVariable(name: "c",
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 18, type: [[DI_TYPEDEF]])
