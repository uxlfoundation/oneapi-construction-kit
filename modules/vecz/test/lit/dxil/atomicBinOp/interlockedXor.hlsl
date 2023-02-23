// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain interlockedXor.hlsl -Fo interlockedXor.bc

// RUN: %veczc -o %t.bc %S/interlockedXor.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<int> a;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  InterlockedXor(a[1].x, 21);
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: [[a:%[a-zA-Z0-9_]*]] = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 0, i32 0, i1 false)
// It isn't really documented anywhere that 3 is the atomic opcode for XOR but it is apparently:
// CHECK: call i32 @dx.op.atomicBinOp.i32(i32 78, %dx.types.Handle [[a]], i32 3, i32 1, i32 0, i32 undef, i32 21)
// CHECK: call i32 @dx.op.atomicBinOp.i32(i32 78, %dx.types.Handle [[a]], i32 3, i32 1, i32 0, i32 undef, i32 21)
// CHECK: call i32 @dx.op.atomicBinOp.i32(i32 78, %dx.types.Handle [[a]], i32 3, i32 1, i32 0, i32 undef, i32 21)
// CHECK: call i32 @dx.op.atomicBinOp.i32(i32 78, %dx.types.Handle [[a]], i32 3, i32 1, i32 0, i32 undef, i32 21)
