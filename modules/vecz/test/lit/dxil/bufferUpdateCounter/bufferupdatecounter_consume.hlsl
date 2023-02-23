// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain bufferupdatecounter_consume.hlsl -Fo bufferupdatecounter_consume.bc

// RUN: %veczc -o %t.bc %S/bufferupdatecounter_consume.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWBuffer<uint> a;
ConsumeStructuredBuffer<uint> b;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = b.Consume();
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: [[a:%[a-zA-Z0-9_]*]] = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 1, i32 1, i1 false)
// CHECK: [[b:%[a-zA-Z0-9_]*]] = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 0, i32 0, i1 false)
// CHECK: call i32 @dx.op.bufferUpdateCounter(i32 70, %dx.types.Handle [[a]], i8 -1)
// CHECK: call i32 @dx.op.bufferUpdateCounter(i32 70, %dx.types.Handle [[a]], i8 -1)
// CHECK: call i32 @dx.op.bufferUpdateCounter(i32 70, %dx.types.Handle [[a]], i8 -1)
// CHECK: call i32 @dx.op.bufferUpdateCounter(i32 70, %dx.types.Handle [[a]], i8 -1)
// CHECK: call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle [[a]]
// CHECK: call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle [[a]]
// CHECK: call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle [[a]]
// CHECK: call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle [[a]]
