// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain bufferupdatecounter_append.hlsl -Fo bufferupdatecounter_append.bc

// RUN: %veczc -o %t.bc %S/bufferupdatecounter_append.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

StructuredBuffer<uint> a;
AppendStructuredBuffer<uint> b;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  b.Append(a[gid.x]);
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: [[b:%[a-zA-Z0-9_]*]] = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 0, i32 0, i1 false)
// CHECK: call i32 @dx.op.bufferUpdateCounter(i32 70, %dx.types.Handle [[b]], i8 1)
// CHECK: call i32 @dx.op.bufferUpdateCounter(i32 70, %dx.types.Handle [[b]], i8 1)
// CHECK: call i32 @dx.op.bufferUpdateCounter(i32 70, %dx.types.Handle [[b]], i8 1)
// CHECK: call i32 @dx.op.bufferUpdateCounter(i32 70, %dx.types.Handle [[b]], i8 1)
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[b]]
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[b]]
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[b]]
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[b]]
