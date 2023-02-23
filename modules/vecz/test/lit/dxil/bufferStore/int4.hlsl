// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain int4.hlsl -Fo int4.bc

// RUN: %veczc -o %t.bc %S/int4.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<int4> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x].xyzw = 42;
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK: [[a:%[a-zA-Z0-9_]*]] = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 0, i32 0, i1 false)
// CHECK: [[x:%[a-zA-Z0-9_]*]] = call i32 @dx.op.threadId.i32(i32 93, i32 0)
// CHECK: [[y:%[a-zA-Z0-9_]*]] = add i32 [[x]], 1
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]], i32 [[x]], i32 0, i32 42, i32 42, i32 42, i32 42, i8 15)
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]], i32 [[y]], i32 0, i32 42, i32 42, i32 42, i32 42, i8 15)
