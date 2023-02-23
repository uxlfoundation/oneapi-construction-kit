// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain atomicCompareExchange.hlsl -Fo atomicCompareExchange.bc

// RUN: %veczc -o %t.bc %S/atomicCompareExchange.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<uint> a;
RWStructuredBuffer<uint> b;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  InterlockedCompareExchange(a[gid.x], 42, b[gid.x], a[gid.x]);
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK: [[b:%[a-zA-Z0-9_]*]] = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 1, i32 1, i1 false)
// CHECK: [[a:%[a-zA-Z0-9_]*]] = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 0, i32 0, i1 false)
// CHECK: [[tid0:%[a-zA-Z0-9_]*]] = call i32 @dx.op.threadId.i32(i32 93, i32 0)
// CHECK: [[tid1:%[a-zA-Z0-9_]*]] = add i32 [[tid0]], 1
// CHECK: call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle [[b]], i32 [[tid0]], i32 0)
// CHECK: call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle [[b]], i32 [[tid1]], i32 0)
// CHECK: [[val0:%[a-zA-Z0-9_]*]] = extractvalue
// CHECK: [[val1:%[a-zA-Z0-9_]*]] = extractvalue
// CHECK: [[compex0:%[a-zA-Z0-9_]*]] = call i32 @dx.op.atomicCompareExchange.i32(i32 79, %dx.types.Handle [[a]], i32 [[tid0]], i32 0, i32 undef, i32 42, i32 [[val0]])
// CHECK: [[compex1:%[a-zA-Z0-9_]*]] = call i32 @dx.op.atomicCompareExchange.i32(i32 79, %dx.types.Handle [[a]], i32 [[tid1]], i32 0, i32 undef, i32 42, i32 [[val1]])
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]], i32 [[tid0]], i32 0, i32 [[compex0]],
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]], i32 [[tid1]], i32 0, i32 [[compex1]],
