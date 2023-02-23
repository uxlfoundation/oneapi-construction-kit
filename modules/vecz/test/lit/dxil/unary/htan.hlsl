// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain htan.hlsl -Fo htan.bc

// RUN: %veczc -o %t.bc %S/htan.bc -k CSMain -w 8
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float> a;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = tanh(a[gid.x]);
}

// CHECK: define void @__vecz_v8_CSMain
// CHECK: call float @dx.op.unary.f32(i32 20,
// CHECK: call float @dx.op.unary.f32(i32 20,
// CHECK: call float @dx.op.unary.f32(i32 20,
// CHECK: call float @dx.op.unary.f32(i32 20,
// CHECK: call float @dx.op.unary.f32(i32 20,
// CHECK: call float @dx.op.unary.f32(i32 20,
// CHECK: call float @dx.op.unary.f32(i32 20,
// CHECK: call float @dx.op.unary.f32(i32 20,
