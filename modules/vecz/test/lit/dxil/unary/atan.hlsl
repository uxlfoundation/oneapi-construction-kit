// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain atan.hlsl -Fo atan.bc

// RUN: %veczc -o %t.bc %S/atan.bc -k CSMain -w 16
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float> a;

[numthreads(32, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = atan(a[gid.x]);
}

// CHECK: define void @__vecz_v16_CSMain
// CHECK: call float @dx.op.unary.f32(i32 17,
// CHECK: call float @dx.op.unary.f32(i32 17,
// CHECK: call float @dx.op.unary.f32(i32 17,
// CHECK: call float @dx.op.unary.f32(i32 17,
// CHECK: call float @dx.op.unary.f32(i32 17,
// CHECK: call float @dx.op.unary.f32(i32 17,
// CHECK: call float @dx.op.unary.f32(i32 17,
// CHECK: call float @dx.op.unary.f32(i32 17,
// CHECK: call float @dx.op.unary.f32(i32 17,
// CHECK: call float @dx.op.unary.f32(i32 17,
// CHECK: call float @dx.op.unary.f32(i32 17,
// CHECK: call float @dx.op.unary.f32(i32 17,
// CHECK: call float @dx.op.unary.f32(i32 17,
// CHECK: call float @dx.op.unary.f32(i32 17,
// CHECK: call float @dx.op.unary.f32(i32 17,
// CHECK: call float @dx.op.unary.f32(i32 17,
