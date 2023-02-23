// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain interlockedAdd.hlsl -Fo interlockedAdd.bc

// RUN: %veczc -o %t.bc %S/interlockedAdd.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<uint> a;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  InterlockedAdd(a[4].x, 42);
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: [[a:%[a-zA-Z0-9_]*]] = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 0, i32 0, i1 false)
// CHECK: call i32 @dx.op.atomicBinOp.i32(i32 78, %dx.types.Handle [[a]], i32 0, i32 4, i32 0, i32 undef, i32 42)
// CHECK: call i32 @dx.op.atomicBinOp.i32(i32 78, %dx.types.Handle [[a]], i32 0, i32 4, i32 0, i32 undef, i32 42)
// CHECK: call i32 @dx.op.atomicBinOp.i32(i32 78, %dx.types.Handle [[a]], i32 0, i32 4, i32 0, i32 undef, i32 42)
// CHECK: call i32 @dx.op.atomicBinOp.i32(i32 78, %dx.types.Handle [[a]], i32 0, i32 4, i32 0, i32 undef, i32 42)
