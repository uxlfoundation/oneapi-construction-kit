// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain frc.hlsl -Fo frc.bc

// RUN: %veczc -o %t.bc %S/frc.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float2> a;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = frac(a[gid.x]);
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: call float @dx.op.unary.f32(i32 22,
// CHECK: call float @dx.op.unary.f32(i32 22,
// CHECK: call float @dx.op.unary.f32(i32 22,
// CHECK: call float @dx.op.unary.f32(i32 22,
// CHECK: call float @dx.op.unary.f32(i32 22,
// CHECK: call float @dx.op.unary.f32(i32 22,
// CHECK: call float @dx.op.unary.f32(i32 22,
// CHECK: call float @dx.op.unary.f32(i32 22,
