// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain asin.hlsl -Fo asin.bc

// RUN: %veczc -o %t.bc %S/asin.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float2x2> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = asin(a[gid.x]);
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK: call float @dx.op.unary.f32(i32 16,
// CHECK: call float @dx.op.unary.f32(i32 16,
// CHECK: call float @dx.op.unary.f32(i32 16,
// CHECK: call float @dx.op.unary.f32(i32 16,
// CHECK: call float @dx.op.unary.f32(i32 16,
// CHECK: call float @dx.op.unary.f32(i32 16,
// CHECK: call float @dx.op.unary.f32(i32 16,
// CHECK: call float @dx.op.unary.f32(i32 16,
