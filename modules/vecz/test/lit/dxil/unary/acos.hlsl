// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain acos.hlsl -Fo acos.bc

// RUN: %veczc -o %t.bc %S/acos.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = acos(a[gid.x]);
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: call float @dx.op.unary.f32(i32 15,
// CHECK: call float @dx.op.unary.f32(i32 15,
// CHECK: call float @dx.op.unary.f32(i32 15,
// CHECK: call float @dx.op.unary.f32(i32 15,
