// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain cbufferloadlegacy_int.hlsl -Fo cbufferloadlegacy_int.bc

// RUN: %veczc -o %t.bc %S/cbufferloadlegacy_int.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<int> a;

cbuffer foo { float b; float c; int d;};

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = d;
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: [[a:%[a-zA-Z0-9_]*]] = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 0, i32 0, i1 false)
// CHECK: [[foo:%[a-zA-Z0-9_]*]] = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 2, i32 0, i32 0, i1 false)
// CHECK: [[load:%[a-zA-Z0-9_]*]] = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle [[foo]], i32 0)
// CHECK: extractvalue %dx.types.CBufRet.i32 [[load]], 2
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]]
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]]
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]]
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]]
