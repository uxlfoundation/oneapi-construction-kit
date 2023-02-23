// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain hcos.hlsl -Fo hcos.bc

// RUN: %veczc -o %t.bc %S/hcos.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float2x3> a;

[numthreads(16, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = cosh(a[gid.x]);
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
// CHECK: call float @dx.op.unary.f32(i32 18,
