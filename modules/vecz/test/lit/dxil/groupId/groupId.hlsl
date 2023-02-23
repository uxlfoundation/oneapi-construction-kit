// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain groupId.hlsl -Fo groupId.bc

// RUN: %veczc -o %t.bc %S/groupId.bc -k CSMain -w 8
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<uint> a;

[numthreads(16, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID, uint3 grpId : SV_GroupID) {
  a[gid.x] = grpId.x + grpId.y + grpId.z;
}

// CHECK: define void @__vecz_v8_CSMain
// CHECK: call i32 @dx.op.groupId.i32(i32 94, i32 0)
// CHECK: call i32 @dx.op.groupId.i32(i32 94, i32 1)
// CHECK: call i32 @dx.op.groupId.i32(i32 94, i32 2)
