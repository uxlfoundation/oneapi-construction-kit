// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain isfinite.hlsl -Fo isfinite.bc

// RUN: %veczc -o %t.bc %S/isfinite.bc -k CSMain -w 8
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<int> a;
StructuredBuffer<float> b;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = isfinite(b[gid.x]);
}

// CHECK: define void @__vecz_v8_CSMain

// CHECK: [[bitcast:%[a-zA-Z0-9_]*]] = bitcast <8 x float> [[in:%[a-zA-Z0-9_]*]] to <8 x i32>
// CHECK: [[and:%[a-zA-Z0-9_]*]] = and <8 x i32> [[bitcast]], <i32 2139095040, i32 2139095040, i32 2139095040, i32 2139095040, i32 2139095040, i32 2139095040, i32 2139095040, i32 2139095040>
// CHECK: [[icmp:%[a-zA-Z0-9_]*]] = icmp ne <8 x i32> [[and]], <i32 2139095040, i32 2139095040, i32 2139095040, i32 2139095040, i32 2139095040, i32 2139095040, i32 2139095040, i32 2139095040>
// CHECK: zext <8 x i1> [[icmp]] to <8 x i32>
